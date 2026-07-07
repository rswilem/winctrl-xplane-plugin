#if IBM
#include "appstate.h"
#include "config.h"
#include "usbdevice.h"

#include <chrono>
#include <hidsdi.h>
#include <iostream>
#include <setupapi.h>
#include <thread>
#include <windows.h>
#include <XPLMUtilities.h>

extern "C" {
#include <hidpi.h>
}

std::string USBDevice::pendingDevicePath;

// Write path resilience tunables. See docs/windows-write-path-fix.md.
// kWriteTimeoutMs   - how long a single overlapped write may stay pending
//                     before it is cancelled and (if not delivered) retried.
// kWriteAttempts    - retries of the SAME packet in place before the device
//                     is declared unhealthy. A retry only happens when we know
//                     the packet was NOT delivered (see the stream invariant).
// kMaxQueuedPackets - backlog ceiling; crossing it means the writer is stuck,
//                     so the device is recycled rather than dropping packets.
static constexpr DWORD kWriteTimeoutMs = 500;
static constexpr int kWriteAttempts = 3;
static constexpr size_t kMaxQueuedPackets = 5000;

USBDevice::USBDevice(HIDDeviceHandle aHidDevice, uint16_t aVendorId, uint16_t aProductId, std::string aVendorName, std::string aProductName) :
    hidDevice(aHidDevice), vendorId(aVendorId), productId(aProductId), vendorName(aVendorName), productName(aProductName), connected(false) {
    devicePath = pendingDevicePath;
    pendingDevicePath.clear();
}

USBDevice::~USBDevice() {
    // Device destructor calls cancelTasksForOwner as a fallback in case a
    // derived product class forgot. Profile destructors call cleanupProfile.
    AppState::getInstance()->cancelTasksForOwner(this);
    disconnect();
}

bool USBDevice::connect() {
    static const size_t kInputReportSize = 65;
    if (inputBuffer) {
        delete[] inputBuffer;
        inputBuffer = nullptr;
    }
    inputBuffer = new uint8_t[kInputReportSize];

    PHIDP_PREPARSED_DATA preparsedData = nullptr;
    if (HidD_GetPreparsedData(hidDevice, &preparsedData)) {
        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS) {
            outputReportByteLength = caps.OutputReportByteLength;
            Logger::getInstance()->debug("Output report byte length: %u\n", outputReportByteLength);
        } else {
            Logger::getInstance()->error("Failed to get HID capabilities\n");
        }
        HidD_FreePreparsedData(preparsedData);
    } else {
        Logger::getInstance()->error("Failed to get preparsed data\n");
    }

    if (hidWriteDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hidWriteDevice);
        hidWriteDevice = INVALID_HANDLE_VALUE;
    }
    if (!devicePath.empty()) {
        // Opened overlapped so the write thread can wait on each write with a
        // timeout and cancel it if the device stops draining its OUT endpoint,
        // instead of blocking forever. Only this dedicated handle is overlapped;
        // the shared hidDevice handle stays non-overlapped.
        hidWriteDevice = CreateFileA(devicePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    }
    if (hidWriteDevice == INVALID_HANDLE_VALUE) {
        // Writes fall back to the shared handle, where they serialize against
        // the blocking ReadFile and throttle to the device's input report rate.
        Logger::getInstance()->error("Failed to open dedicated write handle for %s, falling back to shared handle: %lu\n",
            productName.empty() ? "Unknown" : productName.c_str(), GetLastError());
    }

    connected = true;
    inputThread = std::thread([this]() {
        HANDLE selfHandle = nullptr;
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &selfHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);
        inputThreadHandle = selfHandle;

        uint8_t buffer[65];
        DWORD bytesRead;
        while (connected && hidDevice != INVALID_HANDLE_VALUE) {
            BOOL result = ReadFile(hidDevice, buffer, sizeof(buffer), &bytesRead, nullptr);

            if (result && bytesRead > 0 && connected) {
                InputReportCallback(this, bytesRead, buffer);
            } else if (!result) {
                DWORD error = GetLastError();
                if (error == ERROR_DEVICE_NOT_CONNECTED ||
                    error == ERROR_OPERATION_ABORTED ||
                    error == ERROR_INVALID_HANDLE) {
                    // Fatal read error. If the device was still considered
                    // connected, flag it disconnected so the reaper recycles it
                    // rather than leaving a zombie (input dead, connected true).
                    // During a normal disconnect() teardown connected is already
                    // false, so the guard keeps shutdown semantics unchanged.
                    if (connected.load()) {
                        connected = false;
                    }
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });

    writeThreadRunning = true;
    writeThread = std::thread(&USBDevice::writeThreadLoop, this);

    return true;
}

void USBDevice::InputReportCallback(void *context, DWORD bytesRead, uint8_t *report) {
    auto *self = static_cast<USBDevice *>(context);
    if (!self || !self->connected || !report || bytesRead == 0) {
        return;
    }

    if (self->hidDevice == INVALID_HANDLE_VALUE) {
        return;
    }

    try {
        InputEvent event;
        event.reportId = report[0];
        event.reportData.assign(report, report + bytesRead);
        event.reportLength = (int) bytesRead;

        self->processOnMainThread(event);
    } catch (const std::system_error &e) {
        return;
    } catch (...) {
        Logger::getInstance()->error("Unexpected exception in InputReportCallback\n");
        return;
    }
}

void USBDevice::update() {
    if (!connected) {
        return;
    }

    processQueuedEvents();
}

void USBDevice::disconnect() {
    connected = false;

    // Drain the write queue, then stop the write thread. Bound the drain with a
    // deadline so a wedged write (blocked or pending forever) cannot hang
    // shutdown; on deadline we stop the thread anyway and it discards the rest.
    auto drainDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (writeQueueSize.load() > 0 && writeThreadRunning &&
           std::chrono::steady_clock::now() < drainDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    writeThreadRunning = false;
    writeQueueCV.notify_all();
    // Wake an in-flight overlapped write immediately instead of after a full
    // timeout slice, so the join below is provably bounded.
    if (hidWriteDevice != INVALID_HANDLE_VALUE) {
        CancelIoEx(hidWriteDevice, nullptr);
    }
    if (writeThread.joinable()) {
        writeThread.join();
    }

    if (hidWriteDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hidWriteDevice);
        hidWriteDevice = INVALID_HANDLE_VALUE;
    }

    if (inputThread.joinable()) {
        // CancelIoEx only cancels a ReadFile that is already pending; the
        // input thread may be between its loop condition and the next read,
        // which would then block forever on a quiescent device. Keep
        // cancelling until the thread has actually exited. A null handle
        // means the thread has not reached its first statement yet; it will
        // then see connected == false and exit before reading.
        HANDLE threadHandle = inputThreadHandle.load();
        while (threadHandle && WaitForSingleObject(threadHandle, 50) == WAIT_TIMEOUT) {
            if (hidDevice != INVALID_HANDLE_VALUE) {
                CancelIoEx(hidDevice, nullptr);
            }
            CancelSynchronousIo(threadHandle);
        }
        inputThread.join();
        if (threadHandle) {
            CloseHandle(threadHandle);
        }
        inputThreadHandle = nullptr;
    }

    if (hidDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hidDevice);
        hidDevice = INVALID_HANDLE_VALUE;
    }

    if (inputBuffer) {
        delete[] inputBuffer;
        inputBuffer = nullptr;
    }
}

void USBDevice::forceStateSync() {
    // noop, code does not use partial data
}

bool USBDevice::writeData(std::vector<uint8_t> data) {
    if (hidDevice == INVALID_HANDLE_VALUE || !connected || data.empty()) {
        return false;
    }

    if (data.size() > 1024) {
        Logger::getInstance()->error("Data size too large: %zu bytes\n", data.size());
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(writeQueueMutex);
        if (writeQueue.size() >= kMaxQueuedPackets) {
            // A backlog this large means the writer is stuck or the device
            // stopped draining. Dropping individual packets would corrupt the
            // ordered output stream (see the stream invariant), so the only
            // correct recovery is a full recycle: flag the device unhealthy and
            // let the reaper rebuild it. connected == false makes writeData
            // return early above, so this logs at most once per wedge.
            Logger::getInstance()->error("Write queue overflow for %s (vendorId: 0x%04X, productId: 0x%04X): %zu packets queued, recycling device\n",
                productName.empty() ? "Unknown" : productName.c_str(), vendorId, productId, writeQueue.size());
            connected = false;
            return false;
        }
        writeQueue.push(std::move(data));
        writeQueueSize.store(writeQueue.size());
    }
    writeQueueCV.notify_one();

    return true;
}

void USBDevice::writeThreadLoop() {
    // One manual-reset event, reused for every overlapped write on this thread.
    // Closed on every exit path below.
    HANDLE writeEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    while (writeThreadRunning) {
        std::vector<uint8_t> data;

        {
            std::unique_lock<std::mutex> lock(writeQueueMutex);
            writeQueueCV.wait(lock, [this] {
                return !writeQueue.empty() || !writeThreadRunning;
            });

            if (!writeThreadRunning) {
                break;
            }

            if (!writeQueue.empty()) {
                data = std::move(writeQueue.front());
                writeQueue.pop();
                writeQueueSize.store(writeQueue.size());
            }
        }

        if (data.empty() || hidDevice == INVALID_HANDLE_VALUE || !connected) {
            continue;
        }

        // Use the dedicated overlapped handle when it (and its event) exist;
        // otherwise fall back to the shared non-overlapped handle. Passing an
        // OVERLAPPED to the shared handle is undefined behavior, so the fallback
        // stays fully synchronous (the throttled pre-rewrite mode).
        HANDLE writeHandle;
        bool overlapped;
        if (hidWriteDevice != INVALID_HANDLE_VALUE && writeEvent != nullptr) {
            writeHandle = hidWriteDevice;
            overlapped = true;
        } else {
            writeHandle = hidDevice;
            overlapped = false;
        }

        std::vector<uint8_t> paddedData = data;
        if (outputReportByteLength > 0 && paddedData.size() < outputReportByteLength) {
            paddedData.resize(outputReportByteLength, 0);
        }

        bool unhealthy = false;
        DWORD lastError = 0;

        for (int attempt = 1; attempt <= kWriteAttempts; ++attempt) {
            DWORD bytesTransferred = 0;
            bool delivered = false;
            bool fatal = false;

            if (overlapped) {
                ResetEvent(writeEvent);
                OVERLAPPED ov = {};
                ov.hEvent = writeEvent;
                if (WriteFile(writeHandle, paddedData.data(), (DWORD) paddedData.size(), nullptr, &ov)) {
                    delivered = true; // completed synchronously
                } else {
                    DWORD error = GetLastError();
                    if (error == ERROR_IO_PENDING) {
                        DWORD wait = WaitForSingleObject(writeEvent, kWriteTimeoutMs);
                        if (wait == WAIT_OBJECT_0) {
                            if (GetOverlappedResult(writeHandle, &ov, &bytesTransferred, FALSE)) {
                                delivered = true;
                            } else {
                                error = GetLastError();
                            }
                        } else {
                            // Timeout (or wait failure). Cancel the IRP. It is
                            // MANDATORY to wait for it to finish before the
                            // OVERLAPPED goes out of scope or is reused.
                            CancelIoEx(writeHandle, &ov);
                            if (GetOverlappedResult(writeHandle, &ov, &bytesTransferred, TRUE)) {
                                // It completed while we were cancelling: it WAS
                                // delivered. Do NOT retry; a duplicate corrupts
                                // the stream exactly like a dropped packet.
                                delivered = true;
                            } else {
                                error = GetLastError(); // ERROR_OPERATION_ABORTED expected
                            }
                        }
                    }
                    if (!delivered) {
                        lastError = error;
                        if (error == ERROR_DEVICE_NOT_CONNECTED || error == ERROR_INVALID_HANDLE) {
                            fatal = true;
                        }
                    }
                }
            } else {
                DWORD bytesWritten = 0;
                if (WriteFile(writeHandle, paddedData.data(), (DWORD) paddedData.size(), &bytesWritten, nullptr)) {
                    delivered = true;
                } else {
                    lastError = GetLastError();
                    if (lastError == ERROR_DEVICE_NOT_CONNECTED || lastError == ERROR_INVALID_HANDLE) {
                        fatal = true;
                    }
                }
            }

            if (delivered) {
                break; // move on to the next packet
            }
            if (fatal || attempt >= kWriteAttempts) {
                unhealthy = true;
                break;
            }
            // Not delivered, not fatal, attempts remain: retry the SAME packet
            // in place (same queue position, since it is already dequeued).
        }

        if (unhealthy) {
            const char *errorName = "UNKNOWN";
            if (lastError == ERROR_DEVICE_NOT_CONNECTED) {
                errorName = "DEVICE_NOT_CONNECTED";
            } else if (lastError == ERROR_INVALID_HANDLE) {
                errorName = "INVALID_HANDLE";
            } else if (lastError == ERROR_OPERATION_ABORTED) {
                errorName = "OPERATION_ABORTED";
            } else if (lastError == ERROR_IO_DEVICE) {
                errorName = "IO_DEVICE";
            }

            size_t discarded = 0;
            {
                std::lock_guard<std::mutex> lock(writeQueueMutex);
                // +1 for the packet that just failed and was already dequeued.
                discarded = writeQueue.size() + 1;
                std::queue<std::vector<uint8_t>> empty;
                std::swap(writeQueue, empty);
                writeQueueSize.store(0);
            }
            connected = false;

            Logger::getInstance()->error("Write failed terminally for %s (vendorId: 0x%04X, productId: 0x%04X): %lu (%s), discarding %zu packet(s). Device will be recycled; if this repeats, disconnect and reconnect the device.\n",
                productName.empty() ? "Unknown" : productName.c_str(), vendorId, productId, lastError, errorName, discarded);

            // Do not exit the thread and do not attempt further writes. Wait
            // until disconnect() clears writeThreadRunning, discarding (and
            // re-clearing) anything that races in after connected went false.
            {
                std::unique_lock<std::mutex> lock(writeQueueMutex);
                while (writeThreadRunning) {
                    writeQueueCV.wait(lock, [this] {
                        return !writeThreadRunning || !writeQueue.empty();
                    });
                    if (!writeQueue.empty()) {
                        std::queue<std::vector<uint8_t>> empty;
                        std::swap(writeQueue, empty);
                        writeQueueSize.store(0);
                    }
                }
            }
            break;
        }
    }

    if (writeEvent != nullptr) {
        CloseHandle(writeEvent);
    }
}
#endif

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

USBDevice::USBDevice(HIDDeviceHandle aHidDevice, uint16_t aVendorId, uint16_t aProductId, std::string aVendorName, std::string aProductName) :
    hidDevice(aHidDevice), vendorId(aVendorId), productId(aProductId), vendorName(aVendorName), productName(aProductName), connected(false) {}

USBDevice::~USBDevice() {
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

    connected = true;
    std::thread inputThread([this]() {
        uint8_t buffer[65];
        DWORD bytesRead;
        while (connected && hidDevice != INVALID_HANDLE_VALUE) {
            BOOL result = ReadFile(hidDevice, buffer, sizeof(buffer), &bytesRead, nullptr);

            if (result && bytesRead > 0 && connected) {
                InputReportCallback(this, bytesRead, buffer);
            } else if (!result) {
                DWORD error = GetLastError();
                if (error == ERROR_DEVICE_NOT_CONNECTED) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });
    inputThread.detach();

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
    connected = false; // Stop new items from being enqueued before draining

    // Wait for write queue to drain before disconnecting
    while (writeQueueSize.load() > 0 && writeThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    writeThreadRunning = false;
    writeQueueCV.notify_all();
    if (writeThread.joinable()) {
        writeThread.join();
    }
    // Handle was closed and reset by the write thread itself on exit

    if (hidDevice != INVALID_HANDLE_VALUE) {
        // Give input thread time to exit
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
        Logger::getInstance()->debug("HID device not open, not connected, or empty data\n");
        return false;
    }

    if (data.size() > 1024) {
        Logger::getInstance()->error("Data size too large: %zu bytes\n", data.size());
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(writeQueueMutex);
        writeQueue.push(std::move(data));
        writeQueueSize.store(writeQueue.size());
    }
    writeQueueCV.notify_one();

    return true;
}

void USBDevice::writeThreadLoop() {
    writeThreadNativeHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());

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

        if (!data.empty() && hidDevice != INVALID_HANDLE_VALUE && connected) {
            std::vector<uint8_t> paddedData = data;
            if (outputReportByteLength > 0 && paddedData.size() < outputReportByteLength) {
                paddedData.resize(outputReportByteLength, 0);
            }

            DWORD bytesWritten;
            writeStartTime.store(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                    .count());
            BOOL writeResult = WriteFile(hidDevice, paddedData.data(), (DWORD) paddedData.size(), &bytesWritten, nullptr);
            writeStartTime.store(0);

            if (!writeResult) {
                DWORD error = GetLastError();
                if (error == ERROR_OPERATION_ABORTED) {
                    Logger::getInstance()->debug("WriteFile cancelled (I/O abort) for %s\n",
                        productName.empty() ? "Unknown" : productName.c_str());
                } else {
                    const char *errorName = "UNKNOWN";
                    if (error == ERROR_DEVICE_NOT_CONNECTED) {
                        errorName = "DEVICE_NOT_CONNECTED";
                    } else if (error == ERROR_INVALID_HANDLE) {
                        errorName = "INVALID_HANDLE";
                    } else if (error == ERROR_IO_DEVICE) {
                        errorName = "IO_DEVICE";
                    }
                    Logger::getInstance()->error("WriteFile failed for %s (vendorId: 0x%04X, productId: 0x%04X): %lu (%s)\n",
                        productName.empty() ? "Unknown" : productName.c_str(), vendorId, productId, error, errorName);
                }
            }
        }
    }

    if (writeThreadNativeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(writeThreadNativeHandle);
        writeThreadNativeHandle = INVALID_HANDLE_VALUE;
    }
}

void USBDevice::cancelStuckWriteIfNeeded(int thresholdMs) {
    int64_t startTime = writeStartTime.load();
    if (startTime == 0 || writeThreadNativeHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
                     .count();
    if (nowMs - startTime > thresholdMs) {
        Logger::getInstance()->error("WriteFile stuck for >%dms on %s, cancelling I/O\n",
            thresholdMs, productName.empty() ? "Unknown" : productName.c_str());
        CancelSynchronousIo(writeThreadNativeHandle);
    }
}
#endif

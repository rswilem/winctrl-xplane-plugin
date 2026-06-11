#if LIN
#include "appstate.h"
#include "config.h"
#include "usbdevice.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#include <XPLMUtilities.h>

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

    if (pipe(inputPipe) < 0) {
        // Without the self-pipe there is no way to wake the input thread out
        // of select(), so disconnect() would hang in join(). Fail the connect.
        Logger::getInstance()->error("Failed to create shutdown pipe: %d\n", errno);
        inputPipe[0] = inputPipe[1] = -1;
        return false;
    }

    connected = true;
    inputThread = std::thread([this]() {
        uint8_t buffer[65];
        while (connected && hidDevice >= 0) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(hidDevice, &fds);
            int maxFd = hidDevice + 1;
            if (inputPipe[0] >= 0) {
                FD_SET(inputPipe[0], &fds);
                if (inputPipe[0] + 1 > maxFd) maxFd = inputPipe[0] + 1;
            }

            int ret = select(maxFd, &fds, nullptr, nullptr, nullptr);
            if (ret < 0) {
                if (errno == EINTR) continue;
                Logger::getInstance()->error("Select failed: %d\n", errno);
                break;
            }

            if (inputPipe[0] >= 0 && FD_ISSET(inputPipe[0], &fds)) {
                break; // shutdown signal
            }

            if (FD_ISSET(hidDevice, &fds)) {
                ssize_t bytesRead = read(hidDevice, buffer, sizeof(buffer));
                if (bytesRead > 0 && connected) {
                    InputReportCallback(this, (int)bytesRead, buffer);
                } else if (bytesRead < 0) {
                    Logger::getInstance()->error("Read failed with error: %d\n", errno);
                    break;
                } else {
                    break; // EOF — device disconnected
                }
            }
        }

        Logger::getInstance()->debug("Input thread exiting\n");
    });

    writeThreadRunning = true;
    writeThread = std::thread(&USBDevice::writeThreadLoop, this);

    return true;
}

void USBDevice::InputReportCallback(void *context, int bytesRead, uint8_t *report) {
    auto *self = static_cast<USBDevice *>(context);
    if (!self || !self->connected || !report || bytesRead <= 0) {
        return;
    }

    try {
        InputEvent event;
        event.reportId = report[0];
        event.reportData.assign(report, report + bytesRead);
        event.reportLength = bytesRead;

        self->processOnMainThread(event);
    } catch (const std::system_error &e) {
        // Silently ignore mutex errors that occur during shutdown
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

    // Wake the input thread via the self-pipe so it exits its select() block
    if (inputPipe[1] >= 0) {
        uint8_t c = 0;
        (void)write(inputPipe[1], &c, 1);
    }

    // Drain write queue, then stop the write thread
    while (writeQueueSize.load() > 0 && writeThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    writeThreadRunning = false;
    writeQueueCV.notify_all();
    if (writeThread.joinable()) {
        writeThread.join();
    }

    if (inputThread.joinable()) {
        inputThread.join();
    }

    if (inputPipe[0] >= 0) { close(inputPipe[0]); inputPipe[0] = -1; }
    if (inputPipe[1] >= 0) { close(inputPipe[1]); inputPipe[1] = -1; }

    if (hidDevice >= 0) {
        close(hidDevice);
        hidDevice = -1;
    }

    if (inputBuffer) {
        delete[] inputBuffer;
        inputBuffer = nullptr;
    }

    Logger::getInstance()->debug("Device disconnected\n");
}

void USBDevice::forceStateSync() {
    // noop, code does not use partial data
}

bool USBDevice::writeData(std::vector<uint8_t> data) {
    if (hidDevice < 0 || !connected || data.empty()) {
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

        if (!data.empty() && hidDevice >= 0 && connected) {
            ssize_t bytesWritten = write(hidDevice, data.data(), data.size());
            if (bytesWritten != (ssize_t) data.size()) {
                Logger::getInstance()->error("Raw write failed: %s (wrote %zd of %zu bytes)\n", strerror(errno), bytesWritten, data.size());
            }
        }
    }
}
#endif

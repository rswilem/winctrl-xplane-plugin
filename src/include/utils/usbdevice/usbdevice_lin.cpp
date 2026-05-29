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

    connected = true;
    std::thread inputThread([this]() {
        uint8_t buffer[65];
        while (connected && hidDevice >= 0) {
            ssize_t bytesRead = read(hidDevice, buffer, sizeof(buffer));
            if (bytesRead > 0 && connected) {
                InputReportCallback(this, (int) bytesRead, buffer);
            } else if (bytesRead < 0) {
                // Device error or disconnected
                Logger::getInstance()->error("Read failed with error: %d\n", errno);
                break;
            } else if (bytesRead == 0) {
                // EOF - device disconnected
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        Logger::getInstance()->debug("Input thread exiting\n");
    });
    inputThread.detach();

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
    // Wait for write queue to drain before disconnecting
    while (writeQueueSize.load() > 0 && writeThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    connected = false;
    writeThreadRunning = false;
    writeQueueCV.notify_all();
    if (writeThread.joinable()) {
        writeThread.join();
    }

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

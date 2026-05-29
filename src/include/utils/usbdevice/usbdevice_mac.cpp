#if APL
#include "appstate.h"
#include "config.h"
#include "usbdevice.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <iostream>
#include <XPLMUtilities.h>

USBDevice::USBDevice(HIDDeviceHandle aHidDevice, uint16_t aVendorId, uint16_t aProductId, std::string aVendorName, std::string aProductName) :
    hidDevice(aHidDevice), vendorId(aVendorId), productId(aProductId), vendorName(aVendorName), productName(aProductName), connected(false) {}

USBDevice::~USBDevice() {
    disconnect();
}

bool USBDevice::connect() {
    static const size_t kInputReportSize = 65;

    // Try to open the device - if it's already opened by the manager, this will return kIOReturnExclusiveAccess
    // or succeed if it wasn't opened yet. Either way, we'll have an open device.
    try {
        IOReturn result = IOHIDDeviceOpen(hidDevice, kIOHIDOptionsTypeNone);
        if (result != kIOReturnSuccess && result != kIOReturnExclusiveAccess) {
            throw std::system_error(std::make_error_code(std::errc::io_error), std::string("IOHIDDeviceOpen failed: ") + std::to_string(result));
        }
    } catch (const std::exception &ex) {
        Logger::getInstance()->debug("Failed to open HID device: %s\nError: %s\n", productName.c_str(), ex.what());
        hidDevice = nullptr;
        return false;
    }

    if (hidDevice) {
        hidQueue = IOHIDQueueCreate(kCFAllocatorDefault, hidDevice, kInputReportSize, 0);

        CFArrayRef elements = IOHIDDeviceCopyMatchingElements(hidDevice, nullptr, kIOHIDOptionsTypeNone);
        if (elements) {
            CFIndex count = CFArrayGetCount(elements);
            for (CFIndex i = 0; i < count; i++) {
                IOHIDElementRef elem = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, i);
                if (IOHIDElementGetType(elem) == kIOHIDElementTypeInput_Button) {
                    IOHIDQueueAddElement(hidQueue, elem);
                }
            }
            CFRelease(elements);
        }

        IOHIDQueueScheduleWithRunLoop(hidQueue, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        IOHIDQueueStart(hidQueue);
    }

    connected = true;

    writeThreadRunning = true;
    writeThread = std::thread(&USBDevice::writeThreadLoop, this);

    return true;
}

void USBDevice::update() {
    if (!connected) {
        return;
    }

    if (!hidQueue) {
        return;
    }

    IOHIDValueRef value = nullptr;
    while ((value = IOHIDQueueCopyNextValue(hidQueue))) {
        handleHIDValue(value);
        CFRelease(value);
    }
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

    if (hidQueue) {
        IOHIDQueueStop(hidQueue);
        IOHIDQueueUnscheduleFromRunLoop(hidQueue, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
        CFRelease(hidQueue);
        hidQueue = nullptr;
    }

    if (hidDevice) {
        IOHIDDeviceClose(hidDevice, kIOHIDOptionsTypeNone);
        hidDevice = nullptr;
    }
}

void USBDevice::forceStateSync() {
    if (!connected || !hidDevice) {
        return;
    }

    CFArrayRef elements = IOHIDDeviceCopyMatchingElements(hidDevice, nullptr, 0);
    if (!elements) {
        return;
    }

    for (CFIndex i = 0; i < CFArrayGetCount(elements); i++) {
        IOHIDElementRef element = (IOHIDElementRef) CFArrayGetValueAtIndex(elements, i);
        if (IOHIDElementGetType(element) != kIOHIDElementTypeInput_Button) {
            continue;
        }

        IOHIDValueRef value = nullptr;
        if (IOHIDDeviceGetValue(hidDevice, element, &value) == kIOReturnSuccess) {
            handleHIDValue(value);
        }
    }

    CFRelease(elements);
}

bool USBDevice::writeData(std::vector<uint8_t> data) {
    if (!hidDevice || !connected || data.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(writeQueueMutex);
        if (!connected || !writeThreadRunning) {
            return false;
        }

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

            if (!writeQueue.empty()) {
                data = std::move(writeQueue.front());
                writeQueue.pop();
                writeQueueSize.store(writeQueue.size());
            } else if (!writeThreadRunning) {
                break;
            }
        }

        if (!data.empty() && hidDevice) {
            uint8_t reportID = data[0];
            IOReturn kr = IOHIDDeviceSetReport(hidDevice, kIOHIDReportTypeOutput, reportID, data.data(), data.size());
            if (kr != kIOReturnSuccess) {
                Logger::getInstance()->debug("IOHIDDeviceSetReport failed: %d\n", kr);
            }
        }
    }
}

void USBDevice::handleHIDValue(IOHIDValueRef value) {
    IOHIDElementRef element = IOHIDValueGetElement(value);
    if (!element) {
        return;
    }

    uint32_t reportID = IOHIDElementGetReportID(element);
    if (reportID != 1) {
        return;
    }

    uint32_t hardwareButtonIndex = IOHIDElementGetUsage(element);
    if (hardwareButtonIndex <= 0 || hardwareButtonIndex > UINT16_MAX) {
        return;
    }

    CFIndex len = IOHIDValueGetLength(value);
    if (len == 0) {
        return;
    }
    const uint8_t *data = static_cast<const uint8_t *>(IOHIDValueGetBytePtr(value));

    bool pressed = data[0] == 1;
    didReceiveButton(hardwareButtonIndex - 1, pressed);
}
#endif

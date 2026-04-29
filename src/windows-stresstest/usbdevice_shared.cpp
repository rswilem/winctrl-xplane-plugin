// Cross-platform USBDevice methods and minimal device factory for the stress test.
//
// This replaces the project's usbdevice.cpp (which pulls in all X-Plane products).
// It is compiled together with usbdevice_win.cpp (the actual HID read/write impl)
// and usbcontroller_win.cpp (device enumeration), both referenced from their
// original source paths — no copy or modification of those files.

#include "config.h"
#include "stress_fmc.h"
#include "usbdevice.h"

#include <algorithm>
#include <mutex>

// Button-press notification hook (weak symbol in the main project; normal here).
void notifyButtonPressed(uint16_t /*buttonId*/, uint16_t /*productId*/) {}

// ---------------------------------------------------------------------------
// Device factory — only MCDU product IDs are handled; all others are ignored.
// ---------------------------------------------------------------------------
USBDevice *USBDevice::Device(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId,
    std::string vendorName, std::string productName) {
    if (vendorId != WINCTRL_VENDOR_ID) {
        Logger::getInstance()->debug("Vendor ID mismatch: 0x%04X != 0x%04X\n", vendorId, WINCTRL_VENDOR_ID);
        return nullptr;
    }

    switch (productId) {
        case 0xBB36: // MCDU-32 (Captain)
        case 0xBB3E: // MCDU-32 (First Officer)
        case 0xBB3A: // MCDU-32 (Observer)
            return new StressFMC(hidDevice, vendorId, productId, vendorName, productName);

        default:
            Logger::getInstance()->debug("Skipping non-MCDU device 0x%04X in stress test\n", productId);
            return nullptr;
    }
}

// ---------------------------------------------------------------------------
// Cross-platform USBDevice methods (mirrors usbdevice.cpp without products)
// ---------------------------------------------------------------------------

const char *USBDevice::classIdentifier() {
    return "USBDevice";
}

void USBDevice::blackout() {}

void USBDevice::didReceiveData(int /*reportId*/, uint8_t * /*report*/, int /*reportLength*/) {}

void USBDevice::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t /*count*/) {
    if (pressed) {
        notifyButtonPressed(hardwareButtonIndex, this->productId);
    }
}

void USBDevice::processOnMainThread(const InputEvent &event) {
    if (!connected) {
        return;
    }
    std::lock_guard<std::mutex> lock(eventQueueMutex);
    eventQueue.push(event);
}

void USBDevice::processQueuedEvents() {
    std::lock_guard<std::mutex> lock(eventQueueMutex);
    while (!eventQueue.empty()) {
        InputEvent event = eventQueue.front();
        eventQueue.pop();
        didReceiveData(event.reportId, event.reportData.data(), event.reportLength);
    }
}

size_t USBDevice::getWriteQueueSize() {
    return writeQueueSize.load();
}

int USBDevice::getDisplayUpdateFrameInterval(int minWaitFrames) {
    size_t queueSize = writeQueueSize.load();
    int interval;
    if      (queueSize < 50)   interval = 2;
    else if (queueSize < 250)  interval = 4;
    else if (queueSize < 500)  interval = 8;
    else if (queueSize < 1000) interval = 16;
    else if (queueSize < 2000) interval = 32;
    else                       interval = 100;
    return std::max(interval, minWaitFrames);
}

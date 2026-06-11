#include "usbcontroller.h"

#include "appstate.h"

bool USBController::anyProfileReady() {
    for (auto &device : devices) {
        if (device->profileReady) {
            return true;
        }
    }

    return false;
}

void USBController::connectAllDevices() {
    AppState::getInstance()->executeAfter(0, this, [this]() {
        enumerateDevices();
    });
}

void USBController::disconnectAllDevices() {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (auto ptr : devices) {
        ptr->blackout();
        ptr->disconnect();
        // Drop platform-side path/pending tracking, otherwise the device is
        // considered still present and can never be re-added until reload.
        forgetDevice(ptr);
        delete ptr;
    }
    devices.clear();
}

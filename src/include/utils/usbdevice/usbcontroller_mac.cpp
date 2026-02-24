#if APL

#include "appstate.h"
#include "config.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <iostream>
#include <XPLMUtilities.h>

USBController *USBController::instance = nullptr;

USBController::USBController() {
    hidManager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (CFGetTypeID(hidManager) != IOHIDManagerGetTypeID()) {
        Logger::getInstance()->error("Failed to create IOHIDManager\n");
        return;
    }

    CFMutableDictionaryRef matchingDict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (matchingDict) {
        uint32_t vid = WINCTRL_VENDOR_ID;
        CFNumberRef vidNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vid);
        CFDictionarySetValue(matchingDict, CFSTR(kIOHIDVendorIDKey), vidNum);
        CFRelease(vidNum);
        IOHIDManagerSetDeviceMatching(hidManager, matchingDict);
        CFRelease(matchingDict);
    } else {
        IOHIDManagerSetDeviceMatching(hidManager, nullptr);
    }

    IOHIDManagerOpen(hidManager, kIOHIDOptionsTypeNone);

    IOHIDManagerRegisterDeviceMatchingCallback(hidManager, &USBController::DeviceAddedCallback, this);
    IOHIDManagerRegisterDeviceRemovalCallback(hidManager, &USBController::DeviceRemovedCallback, this);

    IOHIDManagerScheduleWithRunLoop(hidManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

USBController::~USBController() {
    destroy();
}

USBController *USBController::getInstance() {
    if (instance == nullptr) {
        instance = new USBController();
    }

    return instance;
}

void USBController::enumerateDevices() {
    if (!AppState::getInstance()->pluginInitialized) {
        return;
    }

    CFSetRef deviceSet = IOHIDManagerCopyDevices(hidManager);
    if (deviceSet && CFSetGetCount(deviceSet) > 0) {
        CFIndex num = CFSetGetCount(deviceSet);
        IOHIDDeviceRef *deviceArray = (IOHIDDeviceRef *) malloc(sizeof(IOHIDDeviceRef) * num);
        CFSetGetValues(deviceSet, (const void **) deviceArray);
        for (CFIndex i = 0; i < num; ++i) {
            IOHIDDeviceRef hidDevice = deviceArray[i];
            DeviceAddedCallback(this, kIOReturnSuccess, nullptr, hidDevice);
        }
        free(deviceArray);
        CFRelease(deviceSet);
    } else if (deviceSet) {
        CFRelease(deviceSet);
    }
}

void USBController::destroy() {
    for (auto ptr : devices) {
        delete ptr;
    }
    devices.clear();

    if (hidManager) {
        IOHIDManagerClose(hidManager, kIOHIDOptionsTypeNone);
        CFRelease(hidManager);
        hidManager = nullptr;
    }

    instance = nullptr;
}

bool USBController::deviceExistsWithHIDDevice(IOHIDDeviceRef device) {
    for (auto *dev : devices) {
        if (dev->hidDevice == device) {
            return true;
        }
    }
    return false;
}

void USBController::DeviceAddedCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device) {
    if (!AppState::getInstance()->pluginInitialized || result != kIOReturnSuccess || !context || !device) {
        return;
    }

    auto *self = static_cast<USBController *>(context);

    for (auto *dev : self->devices) {
        if (dev->hidDevice == device) {
            return;
        }
    }

    int vendorId = 0, productId = 0;
    char vendorNameBuf[256] = {0};
    char productNameBuf[256] = {0};
    CFTypeRef vidRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDVendorIDKey));
    CFTypeRef pidRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductIDKey));
    CFTypeRef vendorNameRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDManufacturerKey));
    CFTypeRef productNameRef = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
    if (vidRef && CFGetTypeID(vidRef) == CFNumberGetTypeID()) {
        CFNumberGetValue((CFNumberRef) vidRef, kCFNumberIntType, &vendorId);
    }
    if (pidRef && CFGetTypeID(pidRef) == CFNumberGetTypeID()) {
        CFNumberGetValue((CFNumberRef) pidRef, kCFNumberIntType, &productId);
    }
    if (vendorNameRef && CFGetTypeID(vendorNameRef) == CFStringGetTypeID()) {
        CFStringGetCString((CFStringRef) vendorNameRef, vendorNameBuf, sizeof(vendorNameBuf), kCFStringEncodingUTF8);
    }
    if (productNameRef && CFGetTypeID(productNameRef) == CFStringGetTypeID()) {
        CFStringGetCString((CFStringRef) productNameRef, productNameBuf, sizeof(productNameBuf), kCFStringEncodingUTF8);
    }

    std::string vendorNameStr = std::string(vendorNameBuf);
    std::string productNameStr = std::string(productNameBuf);
    vendorNameStr.erase(0, vendorNameStr.find_first_not_of(" \t\n\r"));
    vendorNameStr.erase(vendorNameStr.find_last_not_of(" \t\n\r") + 1);
    productNameStr.erase(0, productNameStr.find_first_not_of(" \t\n\r"));
    productNameStr.erase(productNameStr.find_last_not_of(" \t\n\r") + 1);

    AppState::getInstance()->executeAfter(0, [self, device, vendorId, productId, vendorNameStr, productNameStr]() {
        if (self->deviceExistsWithHIDDevice(device)) {
            return;
        }

        USBDevice *newDevice = USBDevice::Device(device, vendorId, productId, vendorNameStr, productNameStr);
        if (newDevice) {
            self->devices.push_back(newDevice);
        }
    });
}

void USBController::DeviceRemovedCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device) {
    if (result != kIOReturnSuccess || !context || !device) {
        return;
    }

    auto *self = static_cast<USBController *>(context);
    for (auto it = self->devices.begin(); it != self->devices.end();) {
        if ((*it)->hidDevice == device) {
            (*it)->blackout();
            (*it)->disconnect();
            ++it;
        } else {
            ++it;
        }
    }

    AppState::getInstance()->executeAfter(0, [self, device]() {
        for (auto it = self->devices.begin(); it != self->devices.end();) {
            if ((*it)->hidDevice == device || !(*it)->connected) {
                delete *it;
                it = self->devices.erase(it);
            } else {
                ++it;
            }
        }
    });
}
#endif

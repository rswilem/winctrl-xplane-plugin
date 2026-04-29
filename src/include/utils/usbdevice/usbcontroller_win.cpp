#if IBM
#include "appstate.h"
#include "config.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <dbt.h>
#include <functional>
#include <hidsdi.h>
#include <initguid.h>
#include <iostream>
#include <map>
#include <set>
#include <setupapi.h>
#include <thread>
#include <windows.h>

// HID device interface GUID
DEFINE_GUID(GUID_DEVINTERFACE_HID, 0x4D1E55B2, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);

USBController *USBController::instance = nullptr;

static std::map<USBDevice *, std::string> devicePaths;
static std::set<std::pair<uint16_t, uint16_t>> pendingDevices;

USBController::USBController() {
    enumerateDevices();

    std::thread monitorThread([this]() {
        unsigned char tickCount = 0;
        while (!shouldShutdown) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (shouldShutdown) {
                break;
            }
            for (auto *dev : devices) {
                dev->cancelStuckWriteIfNeeded(2000);
            }
            if (++tickCount >= 3) {
                tickCount = 0;
                checkForDeviceChanges();
            }
        }
    });
    monitorThread.detach();
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

void USBController::destroy() {
    shouldShutdown = true;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto ptr : devices) {
        devicePaths.erase(ptr);
        delete ptr;
    }
    devices.clear();
    pendingDevices.clear();

    instance = nullptr;
}

USBDevice *USBController::createDeviceFromHandle(HANDLE hidDevice, const std::string &devicePath) {
    HIDD_ATTRIBUTES attributes = {};
    attributes.Size = sizeof(attributes);

    if (!HidD_GetAttributes(hidDevice, &attributes) || attributes.VendorID != WINCTRL_VENDOR_ID) {
        CloseHandle(hidDevice);
        return nullptr;
    }

    wchar_t vendorName[256] = {};
    wchar_t productName[256] = {};
    HidD_GetManufacturerString(hidDevice, vendorName, sizeof(vendorName));
    HidD_GetProductString(hidDevice, productName, sizeof(productName));

    char vendorNameA[256] = {};
    char productNameA[256] = {};
    WideCharToMultiByte(CP_UTF8, 0, vendorName, -1, vendorNameA, sizeof(vendorNameA), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, productName, -1, productNameA, sizeof(productNameA), nullptr, nullptr);

    USBDevice *device = USBDevice::Device(hidDevice, attributes.VendorID, attributes.ProductID, std::string(vendorNameA), std::string(productNameA));

    if (device) {
        devicePaths[device] = devicePath;
    }
    return device;
}

bool USBController::deviceExistsWithPath(const std::string &devicePath) {
    for (const auto &pair : devicePaths) {
        if (pair.second == devicePath) {
            return true;
        }
    }
    return false;
}

bool USBController::deviceExistsWithVidPid(uint16_t vendorId, uint16_t productId) {
    for (auto *dev : devices) {
        if (dev->vendorId == vendorId && dev->productId == productId) {
            return true;
        }
    }

    if (pendingDevices.find(std::make_pair(vendorId, productId)) != pendingDevices.end()) {
        return true;
    }

    return false;
}

bool USBController::deviceExistsWithHandle(HANDLE hidDevice) {
    for (auto *dev : devices) {
        if (dev->hidDevice == hidDevice) {
            return true;
        }
    }
    return false;
}

void USBController::addDeviceFromHandle(HANDLE hidDevice, const std::string &devicePath) {
    if (hidDevice == INVALID_HANDLE_VALUE) {
        return;
    }

    if (deviceExistsWithPath(devicePath)) {
        CloseHandle(hidDevice);
        return;
    }

    // Get vendor/product ID to track completion
    HIDD_ATTRIBUTES attributes = {};
    attributes.Size = sizeof(attributes);
    if (!HidD_GetAttributes(hidDevice, &attributes)) {
        CloseHandle(hidDevice);
        return;
    }

    uint16_t vendorId = attributes.VendorID;
    uint16_t productId = attributes.ProductID;

    AppState::getInstance()->executeAfter(0, [this, hidDevice, devicePath, vendorId, productId]() {
        USBDevice *device = createDeviceFromHandle(hidDevice, devicePath);
        if (device) {
            devices.push_back(device);
        }

        pendingDevices.erase(std::make_pair(vendorId, productId));
    });
}

void USBController::enumerateHidDevices(std::function<void(HANDLE, const std::string &)> deviceHandler) {
    if (!AppState::getInstance()->pluginInitialized) {
        return;
    }

    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return;
    }

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = {};
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_HID, i, &deviceInterfaceData); i++) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);

        PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(requiredSize);
        deviceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, deviceDetail, requiredSize, nullptr, nullptr)) {
            HANDLE hidDevice = CreateFile(deviceDetail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hidDevice != INVALID_HANDLE_VALUE) {
                deviceHandler(hidDevice, std::string(deviceDetail->DevicePath));
            }
        }
        free(deviceDetail);
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

void USBController::enumerateDevices() {
    enumerateHidDevices([this](HANDLE hidDevice, const std::string &devicePath) {
        HIDD_ATTRIBUTES attributes = {};
        attributes.Size = sizeof(attributes);
        if (HidD_GetAttributes(hidDevice, &attributes) && attributes.VendorID == WINCTRL_VENDOR_ID) {
            if (deviceExistsWithVidPid(attributes.VendorID, attributes.ProductID)) {
                CloseHandle(hidDevice);
                return;
            }

            pendingDevices.insert(std::make_pair(attributes.VendorID, attributes.ProductID));
            addDeviceFromHandle(hidDevice, devicePath);
        } else {
            CloseHandle(hidDevice);
        }
    });
}

void USBController::checkForDeviceChanges() {
    std::vector<std::string> currentDevicePaths;

    enumerateHidDevices([this, &currentDevicePaths](HANDLE hidDevice, const std::string &devicePath) {
        HIDD_ATTRIBUTES attributes = {};
        attributes.Size = sizeof(attributes);
        if (HidD_GetAttributes(hidDevice, &attributes) && attributes.VendorID == WINCTRL_VENDOR_ID) {
            currentDevicePaths.push_back(devicePath);

            if (!deviceExistsWithVidPid(attributes.VendorID, attributes.ProductID)) {
                pendingDevices.insert(std::make_pair(attributes.VendorID, attributes.ProductID));
                addDeviceFromHandle(hidDevice, devicePath);
            } else {
                CloseHandle(hidDevice);
            }
        } else {
            CloseHandle(hidDevice);
        }
    });

    // First pass: disconnect stale devices
    for (auto *dev : devices) {
        auto pathIt = devicePaths.find(dev);
        bool found = false;
        if (pathIt != devicePaths.end()) {
            found = std::find(currentDevicePaths.begin(), currentDevicePaths.end(), pathIt->second) != currentDevicePaths.end();
        }
        if (!found || dev->hidDevice == INVALID_HANDLE_VALUE || !dev->connected) {
            dev->blackout();
            dev->disconnect();
        }
    }

    // Second pass: deferred erase
    AppState::getInstance()->executeAfter(0, [this]() {
        for (auto it = devices.begin(); it != devices.end();) {
            auto pathIt = devicePaths.find(*it);
            bool remove = false;
            if ((*it)->hidDevice == INVALID_HANDLE_VALUE || !(*it)->connected) {
                remove = true;
            }
            if (pathIt != devicePaths.end() &&
                std::find_if(devices.begin(), devices.end(), [&](USBDevice *d) {
                    return devicePaths[d] == pathIt->second;
                }) == devices.end()) {
                remove = true;
            }
            if (remove) {
                if (pathIt != devicePaths.end()) {
                    devicePaths.erase(pathIt);
                }
                delete *it;
                it = devices.erase(it);
            } else {
                ++it;
            }
        }
    });
}
#endif

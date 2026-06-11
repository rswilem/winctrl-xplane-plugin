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

    monitorThread = std::thread([this]() {
        while (!shouldShutdown) {
            std::unique_lock<std::mutex> lock(monitorMutex);
            monitorCV.wait_for(lock, std::chrono::seconds(5), [this] {
                return shouldShutdown.load();
            });
            lock.unlock();
            if (!shouldShutdown) {
                checkForDeviceChanges();
            }
        }
    });
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
    // Set the flag while holding the mutex so the notify cannot fall between
    // the monitor thread's predicate check and its wait, which would delay
    // shutdown by a full wait_for timeout.
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        shouldShutdown = true;
    }
    monitorCV.notify_one();

    if (monitorThread.joinable()) {
        monitorThread.join();
    }

    std::lock_guard<std::mutex> lock(devicesMutex);
    for (auto ptr : devices) {
        devicePaths.erase(ptr);
        delete ptr;
    }
    devices.clear();
    pendingDevices.clear();

    instance = nullptr;
}

void USBController::forgetDevice(USBDevice *device) {
    // Caller holds devicesMutex.
    devicePaths.erase(device);
    pendingDevices.erase(std::make_pair(device->vendorId, device->productId));
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
    } else {
        CloseHandle(hidDevice);
    }
    return device;
}

bool USBController::deviceExistsWithPath(const std::string &devicePath) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (const auto &pair : devicePaths) {
        if (pair.second == devicePath) {
            return true;
        }
    }
    return false;
}

bool USBController::deviceExistsWithVidPid(uint16_t vendorId, uint16_t productId) {
    std::lock_guard<std::mutex> lock(devicesMutex);
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
    std::lock_guard<std::mutex> lock(devicesMutex);
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

    AppState::getInstance()->executeAfter(0, this, [this, hidDevice, devicePath, vendorId, productId]() {
        std::lock_guard<std::mutex> lock(devicesMutex);
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

            {
                std::lock_guard<std::mutex> lock(devicesMutex);
                pendingDevices.insert(std::make_pair(attributes.VendorID, attributes.ProductID));
            }
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
                {
                    std::lock_guard<std::mutex> lock(devicesMutex);
                    pendingDevices.insert(std::make_pair(attributes.VendorID, attributes.ProductID));
                }
                addDeviceFromHandle(hidDevice, devicePath);
            } else {
                CloseHandle(hidDevice);
            }
        } else {
            CloseHandle(hidDevice);
        }
    });

    // Disconnect and erase stale devices on the flight loop. Disconnecting
    // from the monitor thread would race the deferred deletion below: it sets
    // connected = false first and then blocks joining the device threads,
    // during which the deletion task could free the object under it.
    AppState::getInstance()->executeAfter(0, this, [this, currentDevicePaths]() {
        std::lock_guard<std::mutex> lock(devicesMutex);
        for (auto it = devices.begin(); it != devices.end();) {
            USBDevice *dev = *it;
            auto pathIt = devicePaths.find(dev);
            bool found = false;
            if (pathIt != devicePaths.end()) {
                found = std::find(currentDevicePaths.begin(), currentDevicePaths.end(), pathIt->second) != currentDevicePaths.end();
            }

            if (!found || dev->hidDevice == INVALID_HANDLE_VALUE || !dev->connected) {
                dev->blackout();
                dev->disconnect();
                if (pathIt != devicePaths.end()) {
                    devicePaths.erase(pathIt);
                }
                delete dev;
                it = devices.erase(it);
            } else {
                ++it;
            }
        }
    });
}
#endif

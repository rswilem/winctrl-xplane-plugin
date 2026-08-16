#if IBM
#include "appstate.h"
#include "config.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <cctype>
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
// Group keys of live devices, and of the ones still queued for creation.
static std::map<USBDevice *, std::string> deviceGroupKeys;
static std::set<std::string> pendingDevices;

static constexpr const char *kProductWideKeyPrefix = "vidpid";

// Identifies the physical device behind an interface path, so a second
// identical unit gets its own device object instead of being skipped as a
// duplicate of the first.
//
// Paths look like \\?\hid#vid_xxxx&pid_xxxx[&col##]#{instance}#{guid}, where
// {instance} is {prefix}&{parentHash}&{n}&{index}. Every top-level collection
// and every USB interface of one device shares that instance except for its
// trailing index, while two identical units differ in the parent hash, so the
// instance minus its last token is the unit.
static std::string deviceGroupKey(const std::string &devicePath, uint16_t vendorId, uint16_t productId) {
    const std::string suffix = ":" + std::to_string(vendorId) + ":" + std::to_string(productId);

    size_t instanceStart = devicePath.find('#');
    if (instanceStart != std::string::npos) {
        instanceStart = devicePath.find('#', instanceStart + 1);
    }
    if (instanceStart != std::string::npos) {
        size_t instanceEnd = devicePath.find('#', instanceStart + 1);
        std::string instance = devicePath.substr(instanceStart + 1, instanceEnd == std::string::npos ? std::string::npos : instanceEnd - instanceStart - 1);

        size_t lastToken = instance.rfind('&');
        if (lastToken != std::string::npos && lastToken > 0) {
            instance.resize(lastToken);
            for (char &c : instance) {
                c = (char) tolower((unsigned char) c);
            }
            return "instance:" + instance + suffix;
        }
    }

    Logger::getInstance()->debug("Unrecognized device path %s, grouping by product ID instead\n", devicePath.c_str());
    return kProductWideKeyPrefix + suffix;
}

// A product-wide key matches every key of the same product: with no usable
// path the interfaces cannot be told apart, so they must collapse into one
// object rather than risk two objects driving the same hardware.
static bool groupKeysMatch(const std::string &a, const std::string &b) {
    if (a == b) {
        return true;
    }

    // Keys end in ":vendorId:productId"; what precedes it is the discriminator.
    auto vidPidSuffix = [](const std::string &key) {
        size_t product = key.rfind(':');
        if (product == std::string::npos || product == 0) {
            return std::string();
        }

        size_t vendor = key.rfind(':', product - 1);
        return vendor == std::string::npos ? std::string() : key.substr(vendor);
    };

    bool eitherIsProductWide = a.rfind(kProductWideKeyPrefix, 0) == 0 || b.rfind(kProductWideKeyPrefix, 0) == 0;
    return eitherIsProductWide && vidPidSuffix(a) == vidPidSuffix(b);
}

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
        delete ptr;
    }
    devices.clear();
    devicePaths.clear();
    deviceGroupKeys.clear();
    pendingDevices.clear();

    instance = nullptr;
}

void USBController::forgetDevice(USBDevice *device) {
    // Caller holds devicesMutex.
    devicePaths.erase(device);

    auto keyIt = deviceGroupKeys.find(device);
    if (keyIt != deviceGroupKeys.end()) {
        pendingDevices.erase(keyIt->second);
        deviceGroupKeys.erase(keyIt);
    }
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
    wchar_t serialNumber[256] = {};
    HidD_GetManufacturerString(hidDevice, vendorName, sizeof(vendorName));
    HidD_GetProductString(hidDevice, productName, sizeof(productName));
    HidD_GetSerialNumberString(hidDevice, serialNumber, sizeof(serialNumber));

    char vendorNameA[256] = {};
    char productNameA[256] = {};
    char serialNumberA[256] = {};
    WideCharToMultiByte(CP_UTF8, 0, vendorName, -1, vendorNameA, sizeof(vendorNameA), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, productName, -1, productNameA, sizeof(productNameA), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, serialNumber, -1, serialNumberA, sizeof(serialNumberA), nullptr, nullptr);

    USBDevice::pendingDevicePath = devicePath;
    USBDevice *device = USBDevice::Device(hidDevice, attributes.VendorID, attributes.ProductID, std::string(vendorNameA), std::string(productNameA));
    USBDevice::pendingDevicePath.clear();

    if (device) {
        device->serialNumber = std::string(serialNumberA);
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

bool USBController::deviceExistsWithGroupKey(const std::string &groupKey) {
    std::lock_guard<std::mutex> lock(devicesMutex);
    for (const auto &pair : deviceGroupKeys) {
        if (groupKeysMatch(pair.second, groupKey)) {
            return true;
        }
    }

    for (const auto &pending : pendingDevices) {
        if (groupKeysMatch(pending, groupKey)) {
            return true;
        }
    }

    return false;
}

void USBController::addDeviceFromHandle(HANDLE hidDevice, const std::string &devicePath, const std::string &groupKey) {
    if (hidDevice == INVALID_HANDLE_VALUE) {
        return;
    }

    if (deviceExistsWithPath(devicePath)) {
        CloseHandle(hidDevice);
        return;
    }

    AppState::getInstance()->executeAfter(0, this, [this, hidDevice, devicePath, groupKey]() {
        std::lock_guard<std::mutex> lock(devicesMutex);
        USBDevice *device = createDeviceFromHandle(hidDevice, devicePath);
        if (device) {
            devices.push_back(device);
            deviceGroupKeys[device] = groupKey;

            // Two units of the same product must show different group keys here.
            Logger::getInstance()->info("Registered %s (vendorId: 0x%04X, productId: 0x%04X, handler: %s), device group %s\n", device->productName.c_str(), device->vendorId, device->productId, device->classIdentifier(), groupKey.c_str());
        }

        pendingDevices.erase(groupKey);
    });
}

// Calls deviceHandler for every present WINCTRL HID interface with its group
// key; other vendors are filtered out here. The handler owns the handle.
void USBController::enumerateHidDevices(std::function<void(HANDLE, const std::string &, const std::string &)> deviceHandler) {
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
                HIDD_ATTRIBUTES attributes = {};
                attributes.Size = sizeof(attributes);
                std::string devicePath = std::string(deviceDetail->DevicePath);
                if (HidD_GetAttributes(hidDevice, &attributes) && attributes.VendorID == WINCTRL_VENDOR_ID) {
                    deviceHandler(hidDevice, devicePath, deviceGroupKey(devicePath, attributes.VendorID, attributes.ProductID));
                } else {
                    CloseHandle(hidDevice);
                }
            }
        }
        free(deviceDetail);
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

void USBController::enumerateDevices() {
    enumerateHidDevices([this](HANDLE hidDevice, const std::string &devicePath, const std::string &groupKey) {
        if (deviceExistsWithGroupKey(groupKey)) {
            CloseHandle(hidDevice);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            pendingDevices.insert(groupKey);
        }
        addDeviceFromHandle(hidDevice, devicePath, groupKey);
    });
}

void USBController::checkForDeviceChanges() {
    std::vector<std::string> currentDevicePaths;

    enumerateHidDevices([this, &currentDevicePaths](HANDLE hidDevice, const std::string &devicePath, const std::string &groupKey) {
        currentDevicePaths.push_back(devicePath);

        if (deviceExistsWithGroupKey(groupKey)) {
            CloseHandle(hidDevice);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(devicesMutex);
            pendingDevices.insert(groupKey);
        }
        addDeviceFromHandle(hidDevice, devicePath, groupKey);
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
                // Support logs were silent about devices going away, which made
                // an unplug indistinguishable from a device that stopped
                // responding. Name the reason.
                Logger::getInstance()->info("Removing %s (vendorId: 0x%04X, productId: 0x%04X): %s\n",
                    dev->productName.c_str(), dev->vendorId, dev->productId,
                    !found ? "no longer enumerated" : (dev->hidDevice == INVALID_HANDLE_VALUE ? "handle closed" : "flagged disconnected"));

                dev->blackout();
                dev->disconnect();
                // Also drops the group key, or this device could never be re-added.
                forgetDevice(dev);
                delete dev;
                it = devices.erase(it);
            } else {
                ++it;
            }
        }
    });
}
#endif

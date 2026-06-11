#ifndef USBCONTROLLER_H
#define USBCONTROLLER_H

#include "usbdevice.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if APL
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/usb/IOUSBLib.h>
typedef IOHIDManagerRef HIDManagerHandle;
typedef IOHIDDeviceRef HIDDeviceHandle;
#elif IBM
#include <dbt.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <windows.h>
typedef void *HIDManagerHandle;
typedef HANDLE HIDDeviceHandle;
#elif LIN
#include <libudev.h>
typedef struct udev_monitor *HIDManagerHandle;
typedef int HIDDeviceHandle;
#endif

class USBController {
    private:
        HIDManagerHandle hidManager;
        std::atomic<bool> shouldShutdown{false};

        // Guards devices (and the per-platform path/pending tracking) against
        // monitor-thread reads racing flight-loop mutation on Windows/Linux.
        std::mutex devicesMutex;

#if IBM || LIN
        std::thread monitorThread;
#endif
#if IBM
        std::mutex monitorMutex;
        std::condition_variable monitorCV;
#endif

        USBController();
        static USBController *instance;

        void enumerateDevices();
        void forgetDevice(USBDevice *device);

#if APL
        static void DeviceAddedCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device);
        static void DeviceRemovedCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef device);
        bool deviceExistsWithHIDDevice(IOHIDDeviceRef device);
#elif IBM
        void checkForDeviceChanges();
        void enumerateHidDevices(std::function<void(HANDLE, const std::string &)> deviceHandler);
        USBDevice *createDeviceFromHandle(HANDLE hidDevice, const std::string &devicePath);
        bool deviceExistsWithHandle(HANDLE hidDevice);
        bool deviceExistsWithPath(const std::string &devicePath);
        bool deviceExistsWithVidPid(uint16_t vendorId, uint16_t productId);
        void addDeviceFromHandle(HANDLE hidDevice, const std::string &devicePath);
#elif LIN
        static void DeviceAddedCallback(void *context, struct udev_device *device);
        static void DeviceRemovedCallback(void *context, struct udev_device *device);
        void monitorDevices();
        USBDevice *createDeviceFromPath(const std::string &devicePath);
        bool deviceExistsAtPath(const std::string &devicePath);
        void addDeviceFromPath(const std::string &devicePath);
#endif

    public:
        ~USBController();

        std::vector<USBDevice *> devices;
        static USBController *getInstance();
        void destroy();

        bool anyProfileReady();
        void connectAllDevices();
        void disconnectAllDevices();
};

#endif

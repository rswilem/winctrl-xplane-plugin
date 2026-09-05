#ifndef USBDEVICE_H
#define USBDEVICE_H

#include "config.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <typeinfo>
#include <vector>

#if APL
#include <IOKit/hid/IOHIDLib.h>
typedef IOHIDDeviceRef HIDDeviceHandle;
#elif IBM
#include <hidsdi.h>
#include <setupapi.h>
#include <windows.h>
typedef HANDLE HIDDeviceHandle;
#elif LIN
#include <linux/hidraw.h>
typedef int HIDDeviceHandle;
#endif

struct InputEvent {
        int reportId = 0;
        std::vector<uint8_t> reportData;
        int reportLength = 0;
};

// A group of product IDs the user can switch off in the plugins menu, so that
// another tool (MobiFlight, SimAppPro) can own the hardware instead.
struct DeviceFamily {
        // Stored in preferences.ini; must stay stable across releases.
        const char *preferenceKey = nullptr;
        const char *name = nullptr;
        std::vector<uint16_t> productIds;
};

class USBDevice {
    private:
        uint8_t *inputBuffer = nullptr;
        std::queue<InputEvent> eventQueue;
        std::mutex eventQueueMutex;

        std::queue<std::vector<uint8_t>> writeQueue;
        std::mutex writeQueueMutex;
        std::condition_variable writeQueueCV;
        std::thread writeThread;
        std::atomic<bool> writeThreadRunning{false};
        std::atomic<size_t> writeQueueSize{0};

        void processQueuedEvents();
        void writeThreadLoop();

#if APL
        IOHIDQueueRef hidQueue = nullptr;
        void handleHIDValue(IOHIDValueRef value);
#elif IBM
        USHORT outputReportByteLength = 0;
        std::thread inputThread;
        // Win32 handle of the input thread, published by the thread itself.
        // std::thread::native_handle() is a pthread_t under MinGW's posix
        // thread model, so it cannot be used with WaitForSingleObject or
        // CancelSynchronousIo.
        std::atomic<HANDLE> inputThreadHandle{nullptr};
        // Dedicated write handle. Windows serializes synchronous I/O per file
        // object, so a blocking ReadFile pending on hidDevice would stall every
        // WriteFile on that handle until the next input report arrives. Writing
        // through a second file object decouples the two; writes stay fully
        // synchronous and ordered.
        HANDLE hidWriteDevice = INVALID_HANDLE_VALUE;
        std::string devicePath;
        static void InputReportCallback(void *context, DWORD bytesRead, uint8_t *report);
#elif LIN
        std::thread inputThread;
        int inputPipe[2] = {-1, -1};
        static void InputReportCallback(void *context, int bytesRead, uint8_t *report);
        // Marks the device dead and schedules a controller-level recycle +
        // re-enumeration. Safe to call from the input and write threads.
        void handleFatalIOError(const char *what);
#endif
        int profileMatchRetryCounter = 0;

    public:
        USBDevice(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        virtual ~USBDevice();

        HIDDeviceHandle hidDevice;
        std::atomic<bool> connected{false};
#if IBM
        // Device interface path of the next device to be constructed, set by
        // USBController immediately before the USBDevice::Device factory call
        // and consumed by the constructor. Creation is serialized on the
        // flight loop, so a static handoff is safe. Needed because connect()
        // runs inside the product constructor, before the controller could
        // set an instance member.
        static std::string pendingDevicePath;
#endif
#if APL
        // Set when the OS already removed the device: disconnect() must skip
        // IOHIDDeviceClose but still release the retained reference.
        std::atomic<bool> deviceRemoved{false};
#endif
#if LIN
        // Set when the hidraw fd returned a fatal error (device dropped off
        // the bus). USBController::recycleFailedDevices reaps these.
        std::atomic<bool> ioFailed{false};
#endif
        bool profileReady = false;
        uint16_t vendorId;
        uint16_t productId;
        std::string vendorName;
        std::string productName;
        // Set by the platform controller after construction; empty when the
        // device has no serial or it could not be read.
        std::string serialNumber;

        virtual const char *classIdentifier();
        virtual const char *activeProfileName() const;
        virtual bool connect();
        void disconnect();
        virtual void update();
        virtual void didReceiveData(int reportId, uint8_t *report, int reportLength);
        virtual void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1);
        // True when the user assigned this button in X-Plane's joystick
        // settings (globally or in the active aircraft's control profile).
        // X-Plane fires that binding itself, so product didReceiveButton
        // overrides must return early instead of running the built-in action.
        // Logs the override in debug mode. Not implemented in the stresstest
        // build (usbdevice_shared.cpp).
        bool isButtonHandledByXPlane(uint16_t hardwareButtonIndex);

        virtual void blackout();
        virtual void forceStateSync();

        void processOnMainThread(const InputEvent &event);

        bool writeData(std::vector<uint8_t> data);
        size_t getWriteQueueSize();
        int getDisplayUpdateFrameInterval(int minWaitFrames = 0);

        // Throttles the eligibility chain. Keeps retrying rather than giving up,
        // since add-ons can register their datarefs late. True on the first call.
        bool shouldRetryProfileMatch();

        static USBDevice *Device(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);

        // A device the user switched off is never claimed: Device() refuses to
        // create it, so it gets no HID handle, no menu entry and no input or
        // display traffic. Unknown product IDs count as enabled.
        static const std::vector<DeviceFamily> &DeviceFamilies();
        static const DeviceFamily *FamilyForProduct(uint16_t productId);
        static bool IsProductEnabled(uint16_t productId);
        static bool IsFamilyEnabled(const DeviceFamily &family);
        static void SetFamilyEnabled(const DeviceFamily &family, bool enabled);
};

#endif

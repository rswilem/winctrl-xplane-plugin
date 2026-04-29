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
        int reportId;
        std::vector<uint8_t> reportData;
        int reportLength;
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
        IOHIDQueueRef hidQueue;
        void handleHIDValue(IOHIDValueRef value);
#elif IBM
        USHORT outputReportByteLength = 0;
        HANDLE writeThreadNativeHandle = INVALID_HANDLE_VALUE;
        std::atomic<int64_t> writeStartTime{0};
        static void InputReportCallback(void *context, DWORD bytesRead, uint8_t *report);
#elif LIN
        static void InputReportCallback(void *context, int bytesRead, uint8_t *report);
#endif

    public:
        USBDevice(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        virtual ~USBDevice();

        HIDDeviceHandle hidDevice;
        bool connected = false;
        bool profileReady = false;
        uint16_t vendorId;
        uint16_t productId;
        std::string vendorName;
        std::string productName;

        virtual const char *classIdentifier();
        virtual bool connect();
        void disconnect();
        virtual void update();
        virtual void didReceiveData(int reportId, uint8_t *report, int reportLength);
        virtual void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1);

        virtual void blackout();
        virtual void forceStateSync();

        void processOnMainThread(const InputEvent &event);

        bool writeData(std::vector<uint8_t> data);
        size_t getWriteQueueSize();
        int getDisplayUpdateFrameInterval(int minWaitFrames = 0);

        static USBDevice *Device(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);

#if IBM
        void cancelStuckWriteIfNeeded(int thresholdMs);
#endif
};

#endif

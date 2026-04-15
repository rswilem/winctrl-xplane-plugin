#include "usbdevice.h"

#include "appstate.h"
#include "product-agp.h"
#include "product-ecam.h"
#include "product-fcu-efis.h"
#include "product-fmc.h"
#include "product-pap3-mcp.h"
#include "product-pdc.h"
#include "product-ursa-minor-joystick.h"
#include "product-ursa-minor-throttle.h"

#include <XPLMUtilities.h>

// The desktop app overrides this function to get notified of button presses
__attribute__((weak)) void notifyButtonPressed(uint16_t buttonId, uint16_t productId) {}

USBDevice *USBDevice::Device(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) {
    if (vendorId != WINCTRL_VENDOR_ID) {
        Logger::getInstance()->debug("Vendor ID mismatch: 0x%04X != 0x%04X\n", vendorId, WINCTRL_VENDOR_ID);
        return nullptr;
    }

    switch (productId) {
        case 0xBC27: { // URSA MINOR Airline Joystick L
            constexpr uint8_t identifierByte = 0x07;
            return new ProductUrsaMinorJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte);
        }
        case 0xBC28: { // URSA MINOR Airline Joystick R
            constexpr uint8_t identifierByte = 0x08;
            return new ProductUrsaMinorJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte);
        }

        case 0xBB36: { // MCDU-32 (Captain)
            constexpr uint8_t identifierByte = 0x32;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_MCDU, FMCDeviceVariant::VARIANT_CAPTAIN, identifierByte);
        }
        case 0xBB3E: { // MCDU-32 (First Officer)
            constexpr uint8_t identifierByte = 0x32;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_MCDU, FMCDeviceVariant::VARIANT_FIRSTOFFICER, identifierByte);
        }
        case 0xBB3A: { // MCDU-32 (Observer)
            constexpr uint8_t identifierByte = 0x32;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_MCDU, FMCDeviceVariant::VARIANT_OBSERVER, identifierByte);
        }

        case 0xBB35: { // PFP 3N (Captain)
            constexpr uint8_t identifierByte = 0x31;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP3N, FMCDeviceVariant::VARIANT_CAPTAIN, identifierByte);
        }
        case 0xBB39: { // PFP 3N (First Officer)
            constexpr uint8_t identifierByte = 0x31;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP3N, FMCDeviceVariant::VARIANT_FIRSTOFFICER, identifierByte);
        }
        case 0xBB3D: { // PFP 3N (Observer)
            constexpr uint8_t identifierByte = 0x31;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP3N, FMCDeviceVariant::VARIANT_OBSERVER, identifierByte);
        }

        case 0xBB38: { // PFP 4 (Captain)
            constexpr uint8_t identifierByte = 0x34;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP4, FMCDeviceVariant::VARIANT_CAPTAIN, identifierByte);
        }
        case 0xBB40: { // PFP 4 (First Officer)
            constexpr uint8_t identifierByte = 0x34;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP4, FMCDeviceVariant::VARIANT_FIRSTOFFICER, identifierByte);
        }
        case 0xBB3C: { // PFP 4 (Observer)
            constexpr uint8_t identifierByte = 0x34;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP4, FMCDeviceVariant::VARIANT_OBSERVER, identifierByte);
        }

        case 0xBB37: { // PFP 7 (Captain)
            constexpr uint8_t identifierByte = 0x33;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP7, FMCDeviceVariant::VARIANT_CAPTAIN, identifierByte);
        }
        case 0xBB3F: { // PFP 7 (First Officer)
            constexpr uint8_t identifierByte = 0x33;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP7, FMCDeviceVariant::VARIANT_FIRSTOFFICER, identifierByte);
        }
        case 0xBB3B: { // PFP 7 (Observer)
            constexpr uint8_t identifierByte = 0x33;
            return new ProductFMC(hidDevice, vendorId, productId, vendorName, productName, FMCHardwareType::HARDWARE_PFP7, FMCDeviceVariant::VARIANT_OBSERVER, identifierByte);
        }

        case 0xBB10: // FCU only
        case 0xBC1E: // FCU + EFIS-R
        case 0xBC1D: // FCU + EFIS-L
        case 0xBA01: // FCU + EFIS-L + EFIS-R
            return new ProductFCUEfis(hidDevice, vendorId, productId, vendorName, productName);

        case 0xBF0F: // PAP3-MCP
            return new ProductPAP3MCP(hidDevice, vendorId, productId, vendorName, productName);

        case 0xBB61: { // 3N PDC L
            constexpr uint8_t identifierByte = 0x60;
            return new ProductPDC(hidDevice, vendorId, productId, vendorName, productName, PDCDeviceVariant::VARIANT_3N_CAPTAIN, identifierByte);
        }

        case 0xBB62: { // 3N PDC R
            constexpr uint8_t identifierByte = 0x60;
            return new ProductPDC(hidDevice, vendorId, productId, vendorName, productName, PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER, identifierByte);
        }

        case 0xBB51: { // 3M PDC L
            constexpr uint8_t identifierByte = 0x50;
            return new ProductPDC(hidDevice, vendorId, productId, vendorName, productName, PDCDeviceVariant::VARIANT_3M_CAPTAIN, identifierByte);
        }

        case 0xBB52: { // 3M PDC R
            constexpr uint8_t identifierByte = 0x50;
            return new ProductPDC(hidDevice, vendorId, productId, vendorName, productName, PDCDeviceVariant::VARIANT_3M_FIRSTOFFICER, identifierByte);
        }

        case 0xBB70: // ECAM
            return new ProductECAM(hidDevice, vendorId, productId, vendorName, productName);

        case 0xBB80: // AGP
            return new ProductAGP(hidDevice, vendorId, productId, vendorName, productName);

            // Unknown...
            // case 0xBB99: // TCAS
            //     return new ProductTCAS(hidDevice, vendorId, productId, vendorName, productName);

        case 0xB920: // URSA MINOR 32 Throttle Metal L
        case 0xB930: // URSA MINOR 32 Throttle Metal R
            return new ProductUrsaMinorThrottle(hidDevice, vendorId, productId, vendorName, productName);

        default:
            Logger::getInstance()->debug("Unknown WINCTRL device - vendorId: 0x%04X, productId: 0x%04X (%s)\n", vendorId, productId, productName.c_str());
            return nullptr;
    }
}

const char *USBDevice::classIdentifier() {
    return "USBDevice (none)";
}

void USBDevice::blackout() {
    // noop, expect override
}

void USBDevice::didReceiveData(int reportId, uint8_t *report, int reportLength) {
    // noop, expect override
}

void USBDevice::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
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
    if (queueSize < 50) {
        interval = 2;
    } else if (queueSize < 250) {
        interval = 4;
    } else if (queueSize < 500) {
        interval = 8;
    } else if (queueSize < 1000) {
        interval = 16;
    } else if (queueSize < 2000) {
        interval = 32;
    } else {
        interval = 100;
    }

    return std::max(interval, minWaitFrames);
}

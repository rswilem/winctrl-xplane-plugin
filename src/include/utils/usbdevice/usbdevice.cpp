#include "usbdevice.h"

#include "appstate.h"
#include "product-agp.h"
#include "product-ecam.h"
#include "product-fcu-efis.h"
#include "product-fmc.h"
#include "product-joystick.h"
#include "product-nws.h"
#include "product-orion-throttle.h"
#include "product-pap3-mcp.h"
#include "product-pdc.h"
#include "product-rmp.h"
#include "product-tcas.h"
#include "product-ursa-minor-throttle.h"
#include "xplane-bindings.h"

#include <algorithm>
#include <set>
#include <XPLMUtilities.h>

// The desktop app overrides this function to get notified of button presses
__attribute__((weak)) void notifyButtonPressed(uint16_t buttonId, uint16_t productId) {}

// Ordered as the product ID switch in Device() below, so the two stay easy to
// compare. Names match classIdentifier() of the device they cover.
static const std::vector<DeviceFamily> deviceFamilies = {
    {"UrsaMinorJoystickEnabled", "Ursa Minor Joystick", {0xBC27, 0xBC28, 0xBC2A, 0xBC29}},
    {"OrionJoystickEnabled", "Orion Joystick", {0xBEA8}},
    {"MCDUEnabled", "FMC (MCDU)", {0xBB36, 0xBB3E, 0xBB3A}},
    {"PFP3NEnabled", "FMC (PFP3N)", {0xBB35, 0xBB39, 0xBB3D}},
    {"PFP4Enabled", "FMC (PFP4)", {0xBB38, 0xBB40, 0xBB3C}},
    {"PFP7Enabled", "FMC (PFP7)", {0xBB37, 0xBB3F, 0xBB3B}},
    {"FCUEfisEnabled", "FCU-EFIS", {0xBB10, 0xBC1E, 0xBC1D, 0xBA01}},
    {"PAP3MCPEnabled", "PAP3-MCP", {0xBF0F}},
    {"PDCEnabled", "PDC", {0xBB61, 0xBB62, 0xBB51, 0xBB52}},
    {"ECAMEnabled", "ECAM", {0xBB70}},
    {"AGPEnabled", "AGP Metal", {0xBB80}},
    {"TCASEnabled", "TCAS", {0xBB81}},
    {"RMPEnabled", "RMP", {0xBB83, 0xBB84, 0xBB85}},
    {"UrsaMinorThrottleEnabled", "Ursa Minor Throttle", {0xB920, 0xB930}},
    {"NWSEnabled", "NWS", {0xB961}},
    {"OrionThrottleEnabled", "Orion Throttle", {0xBD64}},
};

// Product IDs already reported as disabled; enumeration retries every few
// seconds, so without this the log fills up. Cleared when a family is toggled.
static std::set<uint16_t> loggedDisabledProducts;

const std::vector<DeviceFamily> &USBDevice::DeviceFamilies() {
    return deviceFamilies;
}

const DeviceFamily *USBDevice::FamilyForProduct(uint16_t productId) {
    for (const auto &family : deviceFamilies) {
        if (std::find(family.productIds.begin(), family.productIds.end(), productId) != family.productIds.end()) {
            return &family;
        }
    }

    return nullptr;
}

bool USBDevice::IsFamilyEnabled(const DeviceFamily &family) {
    return AppState::getInstance()->readPreference(family.preferenceKey, "enabled") != "disabled";
}

void USBDevice::SetFamilyEnabled(const DeviceFamily &family, bool enabled) {
    AppState::getInstance()->writePreference(family.preferenceKey, enabled ? "enabled" : "disabled");
    loggedDisabledProducts.clear();
}

static std::string bindingPreferenceKey(const DeviceFamily &family) {
    return std::string(family.preferenceKey) + "XPlaneBindings";
}

bool USBDevice::FamilyUsesXPlaneBindings(const DeviceFamily &family) {
    return AppState::getInstance()->readPreference(bindingPreferenceKey(family), "used") != "ignored";
}

void USBDevice::SetFamilyUsesXPlaneBindings(const DeviceFamily &family, bool uses) {
    AppState::getInstance()->writePreference(bindingPreferenceKey(family), uses ? "used" : "ignored");
}

bool USBDevice::ProductUsesXPlaneBindings(uint16_t productId) {
    const DeviceFamily *family = FamilyForProduct(productId);
    return family == nullptr || FamilyUsesXPlaneBindings(*family);
}

bool USBDevice::IsProductEnabled(uint16_t productId) {
    const DeviceFamily *family = FamilyForProduct(productId);
    return family == nullptr || IsFamilyEnabled(*family);
}

USBDevice *USBDevice::Device(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) {
    if (vendorId != WINCTRL_VENDOR_ID) {
        Logger::getInstance()->debug("Vendor ID mismatch: 0x%04X != 0x%04X\n", vendorId, WINCTRL_VENDOR_ID);
        return nullptr;
    }

    if (!IsProductEnabled(productId)) {
        if (loggedDisabledProducts.insert(productId).second) {
            Logger::getInstance()->info("Device 0x%04X (%s) is switched off in the " FRIENDLY_NAME " menu, leaving it to other software\n", productId, productName.c_str());
        }
        return nullptr;
    }

    switch (productId) {
        case 0xBC27: { // URSA MINOR Airline Joystick L
            constexpr uint8_t identifierByte = 0x07;
            constexpr uint8_t motorCode = 0xBF;
            return new ProductJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte, motorCode);
        }
        case 0xBC28: { // URSA MINOR Airline Joystick R
            constexpr uint8_t identifierByte = 0x08;
            constexpr uint8_t motorCode = 0xBF;
            return new ProductJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte, motorCode);
        }
        case 0xBC2A: { // URSA MINOR Fighter Joystick L
            constexpr uint8_t identifierByte = 0x0A;
            constexpr uint8_t motorCode = 0xBF;
            return new ProductJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte, motorCode);
        }
        case 0xBC29: { // URSA MINOR Fighter Joystick R
            constexpr uint8_t identifierByte = 0x09;
            constexpr uint8_t motorCode = 0xBF;
            return new ProductJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte, motorCode);
        }
        case 0xBEA8: { // WINWING Orion Joystick Base 2 + JGRIP-F16
            constexpr uint8_t identifierByte = 0x01;
            constexpr uint8_t motorCode = 0x00;
            return new ProductJoystick(hidDevice, vendorId, productId, vendorName, productName, identifierByte, motorCode);
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

        case 0xBB81: // TCAS
            return new ProductTCAS(hidDevice, vendorId, productId, vendorName, productName);

        case 0xBB83: // RMP L
        case 0xBB84: // RMP R
        case 0xBB85: // RMP C
            return new ProductRMP(hidDevice, vendorId, productId, vendorName, productName);

        case 0xB920: // URSA MINOR 32 Throttle Metal L
        case 0xB930: // URSA MINOR 32 Throttle Metal R
            return new ProductUrsaMinorThrottle(hidDevice, vendorId, productId, vendorName, productName);

        case 0xB961: // WINCTRL 32 NWS (Nosewheel Steering tiller)
            return new ProductNWS(hidDevice, vendorId, productId, vendorName, productName);

        case 0xBD64: // Orion Throttle Base II + F15EX HANDLE L + F15EX HANDLE R
            return new ProductOrionThrottle(hidDevice, vendorId, productId, vendorName, productName);

            // Not yet implemented devices:
            // 0xB980 = WINCTRL Orion 32 Rudder Pedals Metal
            // 0xBEF0 = WINCTRL Orion Combat Rudder Pedals Metal

        default: {
            // Device creation is retried on every enumeration pass, so log each
            // unsupported product once instead of every few seconds. Creation
            // only ever runs on the flight loop, so no lock is needed.
            static std::set<uint32_t> loggedUnknownProducts;
            if (loggedUnknownProducts.insert(((uint32_t) vendorId << 16) | productId).second) {
                Logger::getInstance()->info("Unknown WINCTRL device - vendorId: 0x%04X, productId: 0x%04X (%s)\n", vendorId, productId, productName.c_str());
            }
            return nullptr;
        }
    }
}

const char *USBDevice::classIdentifier() {
    return "USBDevice (none)";
}

const char *USBDevice::activeProfileName() const {
    return "none";
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

bool USBDevice::isButtonHandledByXPlane(uint16_t hardwareButtonIndex) {
    if (!ProductUsesXPlaneBindings(productId)) {
        return false;
    }

    bool handled = XPlaneBindings::getInstance()->isButtonBound(vendorId, productId, serialNumber, hardwareButtonIndex);
    if (handled) {
        Logger::getInstance()->debug("Button %u on %s (0x%04X:0x%04X, serial %s) is overridden in X-Plane joystick settings; suppressing plugin action\n", hardwareButtonIndex, productName.c_str(), vendorId, productId, serialNumber.empty() ? "unknown" : serialNumber.c_str());
    }
    return handled;
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

bool USBDevice::shouldRetryProfileMatch() {
    static constexpr int kRetryIntervalFrames = 30;

    if (profileMatchRetryCounter > 0) {
        profileMatchRetryCounter--;
        return false;
    }

    profileMatchRetryCounter = kRetryIntervalFrames;
    return true;
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

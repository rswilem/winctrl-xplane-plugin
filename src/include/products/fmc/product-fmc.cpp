#include "product-fmc.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/ff767-fmc-profile.h"
#include "profiles/ff777-fmc-profile.h"
#include "profiles/ixeg733-fmc-profile.h"
#include "profiles/laminar-airbus-fmc-profile.h"
#include "profiles/rotatemd11-fmc-profile.h"
#include "profiles/ssg748-fmc-profile.h"
#include "profiles/toliss-fmc-profile.h"
#include "profiles/xcrafts-fmc-profile.h"
#include "profiles/zibo-fmc-profile.h"
#include "usbcontroller.h"

#include <chrono>
#include <XPLMProcessing.h>

ProductFMC::ProductFMC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, FMCHardwareType hardwareType, FMCDeviceVariant variant, unsigned char identifierByte) : USBDevice(hidDevice, vendorId, productId, vendorName, productName), hardwareType(hardwareType), identifierByte(identifierByte), deviceVariant(variant) {
    profile = nullptr;
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageBytesPerLine, ' '));
    lastUpdateCycle = 0;
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    menuItemId = -1;
    fontsMenuItemId = -1;

    pressedButtonIndices = {};

    connect();
}

ProductFMC::~ProductFMC() {
    blackout();
    if (fontsMenuItemId >= 0) {
        PluginsMenu::getInstance()->removeItem(fontsMenuItemId);
        fontsMenuItemId = -1;
    }
    if (menuItemId >= 0) {
        PluginsMenu::getInstance()->removeItem(menuItemId);
    }
    unloadProfile();
}

void ProductFMC::setProfileForCurrentAircraft() {
    if (TolissFMCProfile::IsEligible()) {
        clearDisplay();
        profile = new TolissFMCProfile(this);
        profileReady = true;
    } else if (LaminarFMCProfile::IsEligible()) {
        clearDisplay();
        profile = new LaminarFMCProfile(this);
        profileReady = true;
    } else if (XCraftsFMCProfile::IsEligible()) {
        clearDisplay();
        profile = new XCraftsFMCProfile(this);
        profileReady = true;
    } else if (ZiboFMCProfile::IsEligible()) {
        clearDisplay();
        profile = new ZiboFMCProfile(this);
        profileReady = true;
    } else if (RotateMD11FMCProfile::IsEligible()) {
        clearDisplay();
        profile = new RotateMD11FMCProfile(this);
        profileReady = true;
    } else if (FlightFactor767FMCProfile::IsEligible()) {
        clearDisplay();
        profile = new FlightFactor767FMCProfile(this);
        profileReady = true;
    } else if (FlightFactor777FMCProfile::IsEligible()) {
        clearDisplay();
        profile = new FlightFactor777FMCProfile(this);
        profileReady = true;
    } else if (SSG748FMCProfile::IsEligible()) {
        clearDisplay();
        profile = new SSG748FMCProfile(this);
        profileReady = true;
    } else if (IXEG733FMCProfile::IsEligible()) {
        clearDisplay();
        profile = new IXEG733FMCProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

const char *ProductFMC::classIdentifier() {
    if (hardwareType == FMCHardwareType::HARDWARE_MCDU) {
        return "FMC (MCDU)";
    } else if (hardwareType == FMCHardwareType::HARDWARE_PFP3N) {
        return "FMC (PFP3N)";
    } else if (hardwareType == FMCHardwareType::HARDWARE_PFP4) {
        return "FMC (PFP4)";
    } else if (hardwareType == FMCHardwareType::HARDWARE_PFP7) {
        return "FMC (PFP7)";
    }

    return "FMC (unknown hardware)";
}

bool ProductFMC::connect() {
    if (USBDevice::connect()) {
        uint8_t col_bg[] = {0x00, 0x00, 0x00};

        writeData({0xf0, 0x0, 0x1, 0x38, identifierByte, 0xbb, 0x0, 0x0, 0x1e, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x18, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x34, 0x0, 0x18, 0x0, 0xe, 0x0, 0x18, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x2, 0x38, 0x0, 0x0, 0x0, 0x1, 0x0, 0x5, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x1, 0x0, 0x6, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x3, 0x38, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0xff, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0xa5, 0xff, 0xff, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x4, 0x38, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0xff, 0xff, 0xff, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0xff, 0x0, 0xff, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x5, 0x38, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x3d, 0xff, 0x0, 0xff, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0x63, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x6, 0x38, 0xff, 0xff, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0xff, 0xff, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x7, 0x38, 0x0, 0x0, 0x2, 0x0, 0x0, 0xff, 0xff, 0xff, 0xb, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x42, 0x5c, 0x61, 0xff, 0xc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x8, 0x38, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x77, 0x77, 0x77, 0xff, 0xd, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x5e, 0x73, 0x79, 0xff, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x9, 0x38, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, col_bg[0], col_bg[1], col_bg[2], 0xff, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0xa5, 0xff, 0xff, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xa, 0x38, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0xff, 0xff, 0xff, 0xff, 0x11, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xb, 0x38, 0xff, 0x12, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x3d, 0xff, 0x0, 0xff, 0x13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xc, 0x38, 0x0, 0x3, 0x0, 0xff, 0x63, 0xff, 0xff, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0xff, 0xff, 0x15, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xd, 0x38, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0xff, 0xff, 0xff, 0x16, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x42, 0x5c, 0x61, 0xff, 0x17, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xe, 0x38, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x77, 0x77, 0x77, 0xff, 0x18, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x5e, 0x73, 0x79, 0xff, 0x19, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0xf, 0x38, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x10, 0x38, 0x1b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x2, 0x0, 0x0, 0x0, 0x1c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, identifierByte, 0xbb, 0x0, 0x0, 0x1a, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
        writeData({0xf0, 0x0, 0x11, 0x12, 0x2, identifierByte, 0xbb, 0x0, 0x0, 0x1c, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});

        setLedBrightness(FMCLed::BACKLIGHT, 128);
        setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 128);
        setLedBrightness(FMCLed::OVERALL_LEDS_BRIGHTNESS, 255);
        setAllLedsEnabled(false);
        showBackground(FMCBackgroundVariant::WINCTRL_LOGO);

        setLedBrightness(FMCLed::MCDU_FAIL, 1);
        setLedBrightness(FMCLed::PFP_FAIL, 1);

        std::vector<MenuItem> menuItems = {
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(FMCLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);
                 AppState::getInstance()->executeAfter(2000, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
        };

        // For now, disable side switching on the PFP since it seems to cause
        // the device to reboot as a MCDU in some cases.
        // The sniffed packets are from an MCDU, which is probably why.
        if (hardwareType == FMCHardwareType::HARDWARE_MCDU) {
            menuItems.push_back(
                {.name = "Device variant", .content = std::vector<MenuItem>{
                                               {.name = "Captain", .checked = deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN, .content = [this](int menuId) {
                                                    setDeviceVariant(FMCDeviceVariant::VARIANT_CAPTAIN);
                                                }},
                                               {.name = "First officer", .checked = deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER, .content = [this](int menuId) {
                                                    setDeviceVariant(FMCDeviceVariant::VARIANT_FIRSTOFFICER);
                                                }},
                                               {.name = "Observer", .checked = deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER, .content = [this](int menuId) {
                                                    setDeviceVariant(FMCDeviceVariant::VARIANT_OBSERVER);
                                                }},
                                           }});
        }

        menuItemId = PluginsMenu::getInstance()->addItem(classIdentifier(), menuItems);
        reloadFontsMenu();

        if (!profile) {
            setProfileForCurrentAircraft();
        }

        return true;
    }

    return false;
}

void ProductFMC::blackout() {
    setLedBrightness(FMCLed::BACKLIGHT, 0);
    setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 0);
    setAllLedsEnabled(false);

    clearDisplay();
}

void ProductFMC::unloadProfile() {
    profileReady = false;

    if (!profile) {
        return;
    }

    delete profile;
    profile = nullptr;
}

void ProductFMC::update() {
    if (!connected) {
        return;
    }

    if (!profile) {
        setProfileForCurrentAircraft();
        return;
    }

    USBDevice::update();

    if (++displayUpdateFrameCounter >= std::max(getDisplayUpdateFrameInterval(), 4)) {
        displayUpdateFrameCounter = 0;
        updatePage();
    }
}

void ProductFMC::didReceiveData(int reportId, uint8_t *report, int reportLength) {
    if (!connected || !profile || !report || reportLength <= 0) {
        return;
    }

    if (reportId != 1 || reportLength < 13) {
        return;
    }

    uint64_t buttonsLo = 0;
    uint32_t buttonsHi = 0;
    for (int i = 0; i < 8; ++i) {
        buttonsLo |= ((uint64_t) report[i + 1]) << (8 * i);
    }
    for (int i = 0; i < 4; ++i) {
        buttonsHi |= ((uint32_t) report[i + 9]) << (8 * i);
    }

    if (buttonsLo == lastButtonStateLo && buttonsHi == lastButtonStateHi) {
        return;
    }

    lastButtonStateLo = buttonsLo;
    lastButtonStateHi = buttonsHi;

    for (int i = 0; i < 96; ++i) {
        bool pressed;

        if (i < 64) {
            pressed = (buttonsLo >> i) & 1;
        } else {
            pressed = (buttonsHi >> (i - 64)) & 1;
        }

        didReceiveButton(i, pressed);
    }
}

void ProductFMC::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    bool pressedButtonIndexExists = pressedButtonIndices.find(hardwareButtonIndex) != pressedButtonIndices.end();
    XPLMCommandPhase command = -1;
    if (pressed && !pressedButtonIndexExists) {
        command = xplm_CommandBegin;
    } else if (pressed && pressedButtonIndexExists) {
        command = xplm_CommandContinue;
    } else if (!pressed && pressedButtonIndexExists) {
        command = xplm_CommandEnd;
    }

    if (command < 0) {
        return;
    }

    if (command == xplm_CommandBegin) {
        pressedButtonIndices.insert(hardwareButtonIndex);
    }

    FMCKey key = FMCHardwareMapping::ButtonIdentifierForIndex(hardwareType, hardwareButtonIndex);
    if (key == FMCKey::INVALID_UNKNOWN) {
        // For reference, we often get: [WINCTRL] Received unknown key from hardwareType 1 - hardwareButtonIndex: 207
        debug("Received unknown key from hardwareType %i - hardwareButtonIndex: %i\n", (int) hardwareType, hardwareButtonIndex);
        return;
    }

    const auto &buttonKeyMap = profile->buttonKeyMap();
    auto it = buttonKeyMap.find(key);
    if (it != buttonKeyMap.end()) {
        profile->buttonPressed(it->second, command);
    }

    if (command == xplm_CommandEnd) {
        pressedButtonIndices.erase(hardwareButtonIndex);
    }
}

void ProductFMC::updatePage(bool forceUpdate) {
    if (!connected || !profile) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    bool shouldUpdate = forceUpdate;

    for (const std::string &dataref : profile->displayDatarefs()) {
        if (!lastUpdateCycle || datarefManager->getCachedLastUpdate(dataref.c_str()) > lastUpdateCycle) {
            shouldUpdate = true;
            break;
        }
    }

    if (shouldUpdate) {
        profile->updatePage(page);
        lastUpdateCycle = XPLMGetCycleNumber();
        draw();
    }
}

void ProductFMC::draw(const std::vector<std::vector<char>> *pagePtr) {
    if (!connected || !profile) {
        return;
    }

    const auto &p = pagePtr ? *pagePtr : page;
    std::vector<uint8_t> buf;

    for (int i = 0; i < ProductFMC::PageLines; ++i) {
        for (int j = 0; j < ProductFMC::PageCharsPerLine; ++j) {
            char color = p[i][j * ProductFMC::PageBytesPerChar];
            bool fontSmall = p[i][j * ProductFMC::PageBytesPerChar + 1];
            auto [dataLow, dataHigh] = dataFromColFont(color, fontSmall);
            buf.push_back(dataLow);
            buf.push_back(dataHigh);

            char val = p[i][j * ProductFMC::PageBytesPerChar + ProductFMC::PageBytesPerChar - 1];
            profile->mapCharacter(&buf, val, fontSmall);
        }
    }

    while (!buf.empty()) {
        size_t maxLength = std::min<size_t>(63, buf.size());
        std::vector<uint8_t> usbBuf(buf.begin(), buf.begin() + maxLength);
        usbBuf.insert(usbBuf.begin(), 0xf2);
        if (maxLength < 63) {
            usbBuf.insert(usbBuf.end(), 63 - maxLength, 0);
        }
        writeData(usbBuf);
        buf.erase(buf.begin(), buf.begin() + maxLength);
    }
}

std::pair<uint8_t, uint8_t> ProductFMC::dataFromColFont(char color, bool fontSmall) {
    if (!profile) {
        return {0x42, 0x00}; // Default white
    }

    const std::map<char, FMCTextColor> &col_map = profile->colorMap();

    auto it = col_map.find(color);
    int value = it != col_map.end() ? it->second : FMCTextColor::COLOR_WHITE;
    if (fontSmall) {
        value += 0x016b;
    }

    return {static_cast<uint8_t>(value & 0xFF), static_cast<uint8_t>((value >> 8) & 0xFF)};
}

void ProductFMC::writeLineToPage(std::vector<std::vector<char>> &page, int line, int pos, const std::string &text, char color, bool fontSmall) {
    if (line < 0 || line >= ProductFMC::PageLines) {
        debug("Not writing line %i: Line number is out of range!\n", line);
        return;
    }
    if (pos < 0 || pos + text.length() > ProductFMC::PageCharsPerLine) {
        debug("Not writing line %i: Position number (%i) is out of range!\n", line, pos);
        return;
    }
    if (text.length() > ProductFMC::PageCharsPerLine) {
        debug("Not writing line %i: Text is too long (%lu) for line.\n", line, text.length());
        return;
    }

    pos = pos * ProductFMC::PageBytesPerChar;
    for (size_t c = 0; c < text.length(); ++c) {
        page[line][pos + c * ProductFMC::PageBytesPerChar] = color;
        page[line][pos + c * ProductFMC::PageBytesPerChar + 1] = fontSmall;
        page[line][pos + c * ProductFMC::PageBytesPerChar + ProductFMC::PageBytesPerChar - 1] = text[c];
    }
}

void ProductFMC::clearDisplay() {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageBytesPerLine, ' '));

    std::vector<uint8_t> blankLine = {};
    blankLine.push_back(0xf2);
    for (int i = 0; i < ProductFMC::PageCharsPerLine; ++i) {
        blankLine.push_back(0x42);
        blankLine.push_back(0x00);
        blankLine.push_back(' ');
    }

    for (int i = 0; i < 16; ++i) {
        writeData(blankLine);
    }
}

void ProductFMC::setFont(FontVariant preferredVariant) {
    std::string fontPreference = AppState::getInstance()->readPreference("FMCFont", "default");

    if (fontPreference == "no_font") {
        return;
    }

    preferredFontVariant = preferredVariant;
    bool shouldLoadDefaultFont = fontPreference == "default";
    if (!shouldLoadDefaultFont && !Font::IsCustomFontAvailable(fontPreference)) {
        debug_force("Font file not found for font '%s'\n", fontPreference.c_str());
        AppState::getInstance()->writePreference("FMCFont", "default");
        shouldLoadDefaultFont = true;
    }

    std::vector<std::vector<unsigned char>> font = {};
    if (shouldLoadDefaultFont) {
        font = Font::GlyphData(preferredVariant, identifierByte, hardwareType);
    } else {
        font = Font::GlyphData(fontPreference, identifierByte, hardwareType);
    }

    if (font.empty()) {
        debug_force("Failed to load font data for font '%s'\n", fontPreference.c_str());
        AppState::getInstance()->writePreference("FMCFont", "default");
        return;
    }

    for (auto &fontBytes : font) {
        writeData(fontBytes);
    }

    showBackground(FMCBackgroundVariant::BLACK);
}

void ProductFMC::showBackground(FMCBackgroundVariant variant) {
    std::vector<uint8_t> data;

    switch (variant) {
        case FMCBackgroundVariant::GRAY:
            data = {0xf0, 0x00, 0x02, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x53, 0x20, 0x07, 0x00};
            break;

        case FMCBackgroundVariant::BLACK:
            data = {0xf0, 0x00, 0x03, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xfd, 0x24, 0x07, 0x00};
            break;

        case FMCBackgroundVariant::RED:
            data = {0xf0, 0x00, 0x04, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x55, 0x29, 0x07, 0x00};
            break;

        case FMCBackgroundVariant::GREEN:
            data = {0xf0, 0x00, 0x06, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xad, 0x95, 0x09, 0x00};
            break;

        case FMCBackgroundVariant::BLUE:
            data = {0xf0, 0x00, 0x07, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xa7, 0x9b, 0x09, 0x00};
            break;

        case FMCBackgroundVariant::YELLOW:
            data = {0xf0, 0x00, 0x08, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x09, 0xa1, 0x09, 0x00};
            break;

        case FMCBackgroundVariant::PURPLE:
            data = {0xf0, 0x00, 0x09, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x05, 0xa7, 0x09, 0x00};
            break;

        case FMCBackgroundVariant::WINCTRL_LOGO:
            data = {0xf0, 0x00, 0x0a, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xd4, 0xac, 0x09, 0x00};
            break;

        default:
            return;
    }

    std::vector<uint8_t> extra = {
        0x00, 0x01, 0x00, 0x00, 0x00, static_cast<uint8_t>(0x0c + (int) variant), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    data.insert(data.end(), extra.begin(), extra.end());

    writeData(data);
}

void ProductFMC::setAllLedsEnabled(bool enable) {
    unsigned char start = FMCLed::_PFP_START;
    unsigned char end = FMCLed::_PFP_END;

    if (hardwareType == FMCHardwareType::HARDWARE_MCDU) {
        start = FMCLed::_MCDU_START;
        end = FMCLed::_MCDU_END;
    }

    for (unsigned char i = start; i <= end; ++i) {
        FMCLed led = static_cast<FMCLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductFMC::setLedBrightness(FMCLed led, uint8_t brightness) {
    if (led > FMCLed::OVERALL_LEDS_BRIGHTNESS && hardwareType == FMCHardwareType::HARDWARE_MCDU && led >= FMCLed::_PFP_START && led <= FMCLed::_PFP_END) {
        // Tried setting a PFP led on MCDU hardware, ignore.
        return;
    } else if (led > FMCLed::OVERALL_LEDS_BRIGHTNESS && hardwareType != FMCHardwareType::HARDWARE_MCDU && led >= FMCLed::_MCDU_START && led <= FMCLed::_MCDU_END) {
        // Tried setting a MCDU led on PFP hardware, ignore.
        return;
    }

    writeData({0x02, identifierByte, 0xbb, 0x00, 0x00, 0x03, 0x49, led, brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductFMC::setDeviceVariant(FMCDeviceVariant variant) {
    if (deviceVariant == variant) {
        return;
    }

    writeData({0x02, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x05, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    writeData({0x02, identifierByte, 0xbb, 0x00, 0x00, 0x08, 0x06, 0xcc, 0x00, 0x00, 0x01, static_cast<uint8_t>(variant), 0xff, 0xff});

    // After writing, disconnect and mark as not ready so USBController will remove us from devices array
    // We disconnect because the device ceases to exist after changing variant.
    profileReady = false;
    disconnect();
}

void ProductFMC::reloadFontsMenu() {
    if (menuItemId < 0) {
        return;
    }

    if (fontsMenuItemId >= 0) {
        PluginsMenu::getInstance()->removeItem(fontsMenuItemId);
        fontsMenuItemId = -1;
    }

    std::vector<std::string> customFontFiles = Font::ReadCustomFontFiles();
    std::string fmcFontPreference = AppState::getInstance()->readPreference("FMCFont", "default");
    std::vector<MenuItem> fontMenuItems = {
        {
            .name = "Managed by plugin",
            .checked = fmcFontPreference == "default",
            .content = [this](int itemId) {
                AppState::getInstance()->writePreference("FMCFont", "default");
                PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                PluginsMenu::getInstance()->setItemChecked(itemId, true);

                setFont(preferredFontVariant);
                updatePage(true);
            },
        },
        {
            .name = "No custom font (reconnect USB)",
            .checked = fmcFontPreference == "no_font",
            .content = [this](int itemId) {
                AppState::getInstance()->writePreference("FMCFont", "no_font");
                PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                PluginsMenu::getInstance()->setItemChecked(itemId, true);

                // No font updating - expect USB reconnect to clear the custom font.
            },
        },
        MenuItem::Separator(),
        {
            .name = "Reload font list",
            .checked = false,
            .content = [this](int itemId) {
                reloadFontsMenu();
            },
        },
    };

    if (customFontFiles.size() > 0) {
        fontMenuItems.push_back(MenuItem::Separator());

        for (const std::string &fontFile : customFontFiles) {
            // Clean up font file name by removing whitespace and control characters
            std::string cleanName = fontFile;
            cleanName.erase(std::remove_if(cleanName.begin(), cleanName.end(),
                                [](unsigned char c) {
                                    return std::isspace(c) || std::iscntrl(c);
                                }),
                cleanName.end());

            fontMenuItems.push_back(
                {
                    .name = cleanName,
                    .checked = fmcFontPreference == fontFile,
                    .content = [this, fontFile](int itemId) {
                        AppState::getInstance()->writePreference("FMCFont", fontFile);
                        PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                        PluginsMenu::getInstance()->setItemChecked(itemId, true);

                        setFont(preferredFontVariant);
                        updatePage(true);
                    },
                });
        }
    }

    fontsMenuItemId = PluginsMenu::getInstance()->addItem("Display font", fontMenuItems, false, menuItemId);
}

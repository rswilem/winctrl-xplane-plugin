#include "product-ecam.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/toliss-ecam-profile.h"

#include <algorithm>
#include <cmath>

ProductECAM::ProductECAM(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductECAM::~ProductECAM() {
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductECAM::classIdentifier() {
    return "ECAM";
}

void ProductECAM::setProfileForCurrentAircraft() {
    if (TolissECAMProfile::IsEligible()) {
        profile = new TolissECAMProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductECAM::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(ECAMLed::BACKLIGHT, 128);
    setLedBrightness(ECAMLed::EMER_CANC_BRIGHTNESS, 128);
    setLedBrightness(ECAMLed::OVERALL_LEDS_BRIGHTNESS, 255);
    setAllLedsEnabled(false);

    setProfileForCurrentAircraft();

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(ECAMLed::BACKLIGHT, 128);
                 setLedBrightness(ECAMLed::EMER_CANC_BRIGHTNESS, 128);
                 setLedBrightness(ECAMLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);
                 AppState::getInstance()->executeAfter(2000, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
        });

    return true;
}

void ProductECAM::blackout() {
    setLedBrightness(ECAMLed::BACKLIGHT, 0);
    setLedBrightness(ECAMLed::EMER_CANC_BRIGHTNESS, 0);
    setLedBrightness(ECAMLed::OVERALL_LEDS_BRIGHTNESS, 0);

    setAllLedsEnabled(false);
}

void ProductECAM::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(ECAMLed::_START);
    unsigned char end = static_cast<unsigned char>(ECAMLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        ECAMLed led = static_cast<ECAMLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductECAM::setLedBrightness(ECAMLed led, uint8_t brightness) {
    writeData({0x02, ProductECAM::IdentifierByte, 0xBB, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductECAM::didReceiveData(int reportId, uint8_t *report, int reportLength) {
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

void ProductECAM::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    auto &buttons = profile->buttonDefs();
    auto it = buttons.find(hardwareButtonIndex);
    if (it == buttons.end()) {
        return;
    }

    const ECAMButtonDef *buttonDef = &it->second;

    if (buttonDef->dataref.empty()) {
        return;
    }

    bool pressedButtonIndexExists = pressedButtonIndices.find(hardwareButtonIndex) != pressedButtonIndices.end();
    if (pressed && !pressedButtonIndexExists) {
        pressedButtonIndices.insert(hardwareButtonIndex);
        profile->buttonPressed(buttonDef, xplm_CommandBegin);
    } else if (pressed && pressedButtonIndexExists) {
        profile->buttonPressed(buttonDef, xplm_CommandContinue);
    } else if (!pressed && pressedButtonIndexExists) {
        pressedButtonIndices.erase(hardwareButtonIndex);
        profile->buttonPressed(buttonDef, xplm_CommandEnd);
    }
}

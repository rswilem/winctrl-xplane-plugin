#include "product-pdc.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/ff777-pdc-profile.h"
#include "profiles/fps748-pdc-profile.h"
#include "profiles/xcrafts-ejets-pdc-profile.h"
#include "profiles/xcrafts-erj-pdc-profile.h"
#include "profiles/zibo-pdc-profile.h"

#include <algorithm>
#include <cmath>

ProductPDC::ProductPDC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, PDCDeviceVariant variant, unsigned char identifierByte) : USBDevice(hidDevice, vendorId, productId, vendorName, productName), identifierByte(identifierByte), deviceVariant(variant) {
    profile = nullptr;
    menuItemId = -1;
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductPDC::~ProductPDC() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductPDC::classIdentifier() {
    return "PDC";
}

const char *ProductPDC::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductPDC::setProfileForCurrentAircraft() {
    if (FPS748PDCProfile::IsEligible()) {
        profile = new FPS748PDCProfile(this);
        profileReady = true;
    } else if (XCraftsErjPDCProfile::IsEligible()) {
        profile = new XCraftsErjPDCProfile(this);
        profileReady = true;
    } else if (XCraftsEjetsPDCProfile::IsEligible()) {
        profile = new XCraftsEjetsPDCProfile(this);
        profileReady = true;
    } else if (ZiboPDCProfile::IsEligible()) {
        profile = new ZiboPDCProfile(this);
        profileReady = true;
    } else if (FF777PDCProfile::IsEligible()) {
        profile = new FF777PDCProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductPDC::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(PDCLed::BACKLIGHT, 128);

    setProfileForCurrentAircraft();

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(PDCLed::BACKLIGHT, 255);
                 AppState::getInstance()->executeAfter(1000, this, [this]() {
                     setLedBrightness(PDCLed::BACKLIGHT, 0);
                     AppState::getInstance()->executeAfter(1000, this, [this]() {
                         setLedBrightness(PDCLed::BACKLIGHT, 128);
                     });
                 });
             }},
        });

    return true;
}

void ProductPDC::blackout() {
    setLedBrightness(PDCLed::BACKLIGHT, 0);
}

void ProductPDC::setLedBrightness(PDCLed led, uint8_t brightness) {
    writeData({0x02, identifierByte, 0xBB, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductPDC::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (profile) {
        profile->update();
    }
}

void ProductPDC::didReceiveData(int reportId, uint8_t *report, int reportLength) {
    if (!connected || !profile || !report || reportLength <= 0) {
        return;
    }

    if (reportId != 1 || reportLength < 13) {
#if DEBUG
//        printf("[%s] Ignoring reportId %d, length %d\n", classIdentifier(), reportId, reportLength);
//        printf("[%s] Data (hex): ", classIdentifier());
//        for (int i = 0; i < reportLength; ++i) {
//            printf("%02X ", report[i]);
//        }
//        printf("\n");
#endif
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

void ProductPDC::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    if (isButtonHandledByXPlane(hardwareButtonIndex)) {
        return;
    }

    bool isDeviceVariant3N = deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || deviceVariant == PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER;

    auto &buttons = profile->buttonDefs();
    auto it = std::find_if(buttons.begin(), buttons.end(), [hardwareButtonIndex, isDeviceVariant3N](const auto &kv) {
        return isDeviceVariant3N ? kv.first.first == hardwareButtonIndex : kv.first.second == hardwareButtonIndex;
    });

    if (it == buttons.end()) {
        return;
    }

    const PDCButtonDef *buttonDef = &it->second;
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

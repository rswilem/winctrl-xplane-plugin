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

namespace {
    bool IsCaptainVariant(PDCDeviceVariant variant) {
        return variant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || variant == PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    }
}

ProductPDC::ProductPDC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, PDCDeviceVariant variant, unsigned char identifierByte) : USBDevice(hidDevice, vendorId, productId, vendorName, productName), identifierByte(identifierByte), hardwareVariant(variant), deviceVariant(variant) {
    profile = nullptr;
    menuItemId = -1;
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    // The product ID only presets the side; a stored menu choice always wins, so
    // a unit left on L in SimAppPro can still drive the first officer side.
    std::string variantPreference = AppState::getInstance()->readPreference(variantPreferenceKey(), IsCaptainVariant(variant) ? "captain" : "first_officer");
    if (variantPreference == "first_officer") {
        deviceVariant = isHardware3N() ? PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER : PDCDeviceVariant::VARIANT_3M_FIRSTOFFICER;
    } else {
        deviceVariant = isHardware3N() ? PDCDeviceVariant::VARIANT_3N_CAPTAIN : PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    }

    connect();
}

ProductPDC::~ProductPDC() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    unloadProfile();
}

const char *ProductPDC::classIdentifier() {
    switch (hardwareVariant) {
        case PDCDeviceVariant::VARIANT_3N_CAPTAIN:
            return "PDC 3N (L)";
        case PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER:
            return "PDC 3N (R)";
        case PDCDeviceVariant::VARIANT_3M_CAPTAIN:
            return "PDC 3M (L)";
        case PDCDeviceVariant::VARIANT_3M_FIRSTOFFICER:
            return "PDC 3M (R)";
    }

    return "PDC";
}

std::string ProductPDC::positionName() const {
    switch (hardwareVariant) {
        case PDCDeviceVariant::VARIANT_3N_CAPTAIN:
            return "3N-L";
        case PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER:
            return "3N-R";
        case PDCDeviceVariant::VARIANT_3M_CAPTAIN:
            return "3M-L";
        case PDCDeviceVariant::VARIANT_3M_FIRSTOFFICER:
            return "3M-R";
    }

    return "3N-L";
}

std::string ProductPDC::variantPreferenceKey() const {
    return std::string("PDCVariant") + positionName();
}

bool ProductPDC::isCaptainSide() const {
    return IsCaptainVariant(deviceVariant);
}

bool ProductPDC::isHardware3N() const {
    return hardwareVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || hardwareVariant == PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER;
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

void ProductPDC::unloadProfile() {
    profileReady = false;

    if (profile) {
        // Phased commands latch on CommandBegin, so a button that is still held
        // has to be released on the profile that began it.
        for (int hardwareButtonIndex : pressedButtonIndices) {
            const PDCButtonDef *buttonDef = buttonDefForIndex(hardwareButtonIndex);
            if (buttonDef) {
                profile->buttonPressed(buttonDef, xplm_CommandEnd);
            }
        }

        delete profile;
        profile = nullptr;
    }

    pressedButtonIndices.clear();

    // The latched selectors (VOR L/R, baro mode, mins mode, map mode, range)
    // only report on change, so a stale latch would keep the next profile from
    // ever learning where the switches physically are.
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
}

void ProductPDC::setDeviceVariant(PDCDeviceVariant variant) {
    if (deviceVariant == variant) {
        return;
    }

    // The profiles bake the side into their dataref names and monitor them from
    // their constructor, so the switch needs a fresh profile.
    unloadProfile();

    deviceVariant = variant;
    AppState::getInstance()->writePreference(variantPreferenceKey(), isCaptainSide() ? "captain" : "first_officer");

    setProfileForCurrentAircraft();
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
            PluginsMenu::deviceEnabledItem(productId),
            MenuItem::Separator(),
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(PDCLed::BACKLIGHT, 255);
                 AppState::getInstance()->executeAfter(1000, this, [this]() {
                     setLedBrightness(PDCLed::BACKLIGHT, 0);
                     AppState::getInstance()->executeAfter(1000, this, [this]() {
                         setLedBrightness(PDCLed::BACKLIGHT, 128);
                     });
                 });
             }},
            {.name = "Variant", .content = std::vector<MenuItem>{
                                    {.name = "Captain", .checked = isCaptainSide(), .content = [this](int menuId) {
                                         setDeviceVariant(isHardware3N() ? PDCDeviceVariant::VARIANT_3N_CAPTAIN : PDCDeviceVariant::VARIANT_3M_CAPTAIN);
                                         PluginsMenu::getInstance()->uncheckSubmenuSiblings(menuId);
                                         PluginsMenu::getInstance()->setItemChecked(menuId, true);
                                     }},
                                    {.name = "First officer", .checked = !isCaptainSide(), .content = [this](int menuId) {
                                         setDeviceVariant(isHardware3N() ? PDCDeviceVariant::VARIANT_3N_FIRSTOFFICER : PDCDeviceVariant::VARIANT_3M_FIRSTOFFICER);
                                         PluginsMenu::getInstance()->uncheckSubmenuSiblings(menuId);
                                         PluginsMenu::getInstance()->setItemChecked(menuId, true);
                                     }},
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

void ProductPDC::forceStateSync() {
    pressedButtonIndices.clear();
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;

    USBDevice::forceStateSync();
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

const PDCButtonDef *ProductPDC::buttonDefForIndex(uint16_t hardwareButtonIndex) const {
    if (!profile) {
        return nullptr;
    }

    bool is3N = isHardware3N();
    auto &buttons = profile->buttonDefs();
    auto it = std::find_if(buttons.begin(), buttons.end(), [hardwareButtonIndex, is3N](const auto &kv) {
        return is3N ? kv.first.first == hardwareButtonIndex : kv.first.second == hardwareButtonIndex;
    });

    return it == buttons.end() ? nullptr : &it->second;
}

void ProductPDC::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    if (isButtonHandledByXPlane(hardwareButtonIndex)) {
        return;
    }

    const PDCButtonDef *buttonDef = buttonDefForIndex(hardwareButtonIndex);
    if (!buttonDef || buttonDef->dataref.empty()) {
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

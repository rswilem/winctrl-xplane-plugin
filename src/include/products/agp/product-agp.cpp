#include "product-agp.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "display-frame.h"
#include "plugins-menu.h"
#include "profiles/pa28-agp-profile.h"
#include "profiles/rotatemd11-agp-profile.h"
#include "profiles/toliss-agp-profile.h"
#include "profiles/xcrafts-ejets-agp-profile.h"
#include "profiles/xcrafts-erj-agp-profile.h"
#include "profiles/zibo-agp-profile.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <XPLMUtilities.h>

ProductAGP::ProductAGP(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    profile = nullptr;
    menuItemId = -1;
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductAGP::~ProductAGP() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductAGP::classIdentifier() {
    return "AGP Metal";
}

const char *ProductAGP::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductAGP::setProfileForCurrentAircraft() {
    if (XCraftsEjetsAGPProfile::IsEligible()) {
        profile = new XCraftsEjetsAGPProfile(this);
        profileReady = true;
    } else if (RotateMD11AGPProfile::IsEligible()) {
        profile = new RotateMD11AGPProfile(this);
        profileReady = true;
    } else if (XCraftsErjAGPProfile::IsEligible()) {
        profile = new XCraftsErjAGPProfile(this);
        profileReady = true;
    } else if (ZiboAGPProfile::IsEligible()) {
        profile = new ZiboAGPProfile(this);
        profileReady = true;
    } else if (TolissAGPProfile::IsEligible()) {
        profile = new TolissAGPProfile(this);
        profileReady = true;
    } else if (PA28AGPProfile::IsEligible()) {
        profile = new PA28AGPProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductAGP::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(AGPLed::BACKLIGHT, 128);
    setLedBrightness(AGPLed::LCD_BRIGHTNESS, 128);
    setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, 255);
    setAllLedsEnabled(false);

    setProfileForCurrentAircraft();

    std::string terrainPreference = AppState::getInstance()->readPreference("AGPTerrainND", "first_officer");
    if (terrainPreference == "captain") {
        terrainNDPreference = AGPTerrainNDPreference::CAPTAIN;
    } else if (terrainPreference == "both") {
        terrainNDPreference = AGPTerrainNDPreference::BOTH;
    } else {
        terrainNDPreference = AGPTerrainNDPreference::FIRST_OFFICER;
    }

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            PluginsMenu::deviceEnabledItem(productId),
            MenuItem::Separator(),
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(AGPLed::BACKLIGHT, 128);
                 setLedBrightness(AGPLed::LCD_BRIGHTNESS, 255);
                 setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);

                 AppState::getInstance()->executeAfter(2000, this, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
            {.name = "Terrain on ND", .content = std::vector<MenuItem>{
                                          {.name = "ND1 (Captain)", .checked = terrainPreference == "captain", .content = [this](int itemId) {
                                               AppState::getInstance()->writePreference("AGPTerrainND", "captain");
                                               terrainNDPreference = AGPTerrainNDPreference::CAPTAIN;
                                               PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                               PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                           }},
                                          {.name = "ND2 (First officer)", .checked = terrainPreference == "first_officer", .content = [this](int itemId) {
                                               AppState::getInstance()->writePreference("AGPTerrainND", "first_officer");
                                               terrainNDPreference = AGPTerrainNDPreference::FIRST_OFFICER;
                                               PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                               PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                           }},
                                          {.name = "ND1 + ND2 (Both)", .checked = terrainPreference == "both", .content = [this](int itemId) {
                                               AppState::getInstance()->writePreference("AGPTerrainND", "both");
                                               terrainNDPreference = AGPTerrainNDPreference::BOTH;
                                               PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                               PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                           }},
                                      }},
        });

    return true;
}

void ProductAGP::blackout() {
    setLedBrightness(AGPLed::BACKLIGHT, 0);
    setLedBrightness(AGPLed::LCD_BRIGHTNESS, 0);
    setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, 0);

    setAllLedsEnabled(false);
}

void ProductAGP::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (++displayUpdateFrameCounter >= getDisplayUpdateFrameInterval(12)) {
        displayUpdateFrameCounter = 0;

        if (profile) {
            profile->updateDisplays();
        }
    }
}

void ProductAGP::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(AGPLed::_START);
    unsigned char end = static_cast<unsigned char>(AGPLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        AGPLed led = static_cast<AGPLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductAGP::setLedBrightness(AGPLed led, uint8_t brightness) {
    writeData({0x02, ProductAGP::IdentifierByte, 0xBB, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductAGP::setLCDText(const std::string &chrono, const std::string &utcTime, const std::string &elapsedTime) {
    std::vector<uint8_t> packet = DisplayFrame::buildDataFrame(packetNumber, 0x35, ProductAGP::IdentifierByte, 0xBB);

    const int segmentRowOffsets[7] = {25, 29, 33, 37, 41, 45, 49};
    const int colonRowOffset = 53;

    std::string allDigits;
    uint16_t colonMask = 0;

    SegmentDisplay::parseSegmentText(chrono, 4, allDigits, colonMask, 0, SegmentDisplay::DotPlacement::DualDot);
    SegmentDisplay::parseSegmentText(utcTime, 6, allDigits, colonMask, 4, SegmentDisplay::DotPlacement::DualDot);
    SegmentDisplay::parseSegmentText(elapsedTime, 4, allDigits, colonMask, 10, SegmentDisplay::DotPlacement::DualDot);

    SegmentDisplay::encodeBitplane(packet, allDigits, colonMask, segmentRowOffsets, 7, colonRowOffset);

    writeData(packet);

    writeData(DisplayFrame::buildCommitFrame(packetNumber, ProductAGP::IdentifierByte, 0xBB));
    DisplayFrame::advancePacketNumber(packetNumber);
}

void ProductAGP::didReceiveData(int reportId, uint8_t *report, int reportLength) {
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

void ProductAGP::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    if (isButtonHandledByXPlane(hardwareButtonIndex)) {
        return;
    }

    auto &buttons = profile->buttonDefs();
    auto it = buttons.find(hardwareButtonIndex);
    if (it == buttons.end()) {
        return;
    }

    const AGPButtonDef *buttonDef = &it->second;

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

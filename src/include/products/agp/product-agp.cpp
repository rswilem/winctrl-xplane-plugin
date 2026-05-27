#include "product-agp.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "plugins-menu.h"
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
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductAGP::~ProductAGP() {
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
    } else if (XCraftsErjAGPProfile::IsEligible()) {
        profile = new XCraftsErjAGPProfile(this);
        profileReady = true;
    } else if (ZiboAGPProfile::IsEligible()) {
        profile = new ZiboAGPProfile(this);
        profileReady = true;
    } else if (TolissAGPProfile::IsEligible()) {
        profile = new TolissAGPProfile(this);
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
    } else {
        terrainNDPreference = AGPTerrainNDPreference::FIRST_OFFICER;
    }

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(AGPLed::BACKLIGHT, 128);
                 setLedBrightness(AGPLed::LCD_BRIGHTNESS, 255);
                 setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);

                 AppState::getInstance()->executeAfter(2000, [this]() {
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

void ProductAGP::parseSegment(const std::string &text, int expectedLength, std::string &outDigits, uint16_t &colonMask, int digitOffset) {
    std::string digits;
    uint16_t localColonMask = 0;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c == ':' || c == '.') {
            if (!digits.empty()) {
                if (expectedLength >= 6) {
                    // Standard: Enable bit for digit before (Left) and digit after (Right)

                    if (c == ':') {
                        localColonMask |= (1 << (digits.length() - 1)); // Upper Dot
                    }

                    localColonMask |= (1 << digits.length()); // Lower Dot
                } else {
                    // 4-Digit Displays: Enable bit for digit after (Right) and next digit (Right + 1)
                    // The digit 'digits.length()' is the one we are about to add next.

                    if (c == ':') {
                        localColonMask |= (1 << digits.length()); // Upper Dot
                    }

                    localColonMask |= (1 << (digits.length() + 1)); // Lower Dot
                }
            }
        } else {
            digits += c;
        }
    }

    // Calculate padding amount before modifying digits
    int paddingAmount = 0;
    if (digits.length() < static_cast<size_t>(expectedLength)) {
        paddingAmount = expectedLength - static_cast<int>(digits.length());
    }

    // Shift colon positions by padding amount and add to global mask with offset
    colonMask |= (localColonMask << paddingAmount) << digitOffset;

    // Pad or truncate to expected length
    while (digits.length() < static_cast<size_t>(expectedLength)) {
        digits = ' ' + digits; // Left-pad with spaces
    }
    if (digits.length() > static_cast<size_t>(expectedLength)) {
        digits = digits.substr(digits.length() - expectedLength);
    }
    outDigits += digits;
}

void ProductAGP::setLCDText(const std::string &chrono, const std::string &utcTime, const std::string &elapsedTime) {
    std::vector<uint8_t> packet = {
        0xF0, 0x00, packetNumber, 0x35, ProductAGP::IdentifierByte,
        0xBB, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    packet.resize(64, 0x00);

    const int rowOffsets[8] = {25, 29, 33, 37, 41, 45, 49, 53};

    std::string allDigits;
    uint16_t colonMask = 0;

    parseSegment(chrono, 4, allDigits, colonMask, 0);
    parseSegment(utcTime, 6, allDigits, colonMask, 4);
    parseSegment(elapsedTime, 4, allDigits, colonMask, 10);

    for (int digitIndex = 0; digitIndex < 14; ++digitIndex) {
        char c = allDigits[digitIndex];
        uint8_t charMask = SegmentDisplay::getSegmentMask(c);

        for (int segIndex = 0; segIndex < 7; ++segIndex) {
            if (charMask & (1 << segIndex)) {
                int byteOffset = rowOffsets[segIndex] + (digitIndex / 8);

                int bitPos = digitIndex % 8;

                packet[byteOffset] |= (1 << bitPos);
            }
        }

        if (colonMask & (1 << digitIndex)) {
            int byteOffset = rowOffsets[7] + (digitIndex / 8);
            int bitPos = digitIndex % 8;
            packet[byteOffset] |= (1 << bitPos);
        }
    }

    writeData(packet);

    std::vector<uint8_t> commitPacket = {
        0xF0, 0x00, packetNumber, 0x11, ProductAGP::IdentifierByte,
        0xBB, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    commitPacket.resize(64, 0x00);
    writeData(commitPacket);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }
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

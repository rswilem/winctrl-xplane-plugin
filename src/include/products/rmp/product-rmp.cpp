#include "product-rmp.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/toliss-rmp-profile.h"
#include "profiles/ff777-rmp-profile.h"
#include "segment-display.h"

#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

ProductRMP::ProductRMP(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, RMPDeviceVariant variant, uint8_t identifierByte) : USBDevice(hidDevice, vendorId, productId, vendorName, productName), identifierByte(identifierByte), deviceVariant(variant) {
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};
    lastUpdateCycle = 0;

    connect();
}

ProductRMP::~ProductRMP() {
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductRMP::classIdentifier() {
    switch (deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "RMP (Captain)";
        case RMPDeviceVariant::VARIANT_STBY:
            return "RMP (Stby)";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "RMP (First Officer)";
    }
    return "RMP";
}

const char *ProductRMP::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductRMP::setProfileForCurrentAircraft() {
    if (TolissRMPProfile::IsEligible()) {
        profile = new TolissRMPProfile(this);
        profileReady = true;
    } else if (FF777RMPProfile::IsEligible()) {
        profile = new FF777RMPProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductRMP::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    lastUpdateCycle = 0;
    lastLedBrightness.clear();

    setLedBrightness(RMPLed::BACKLIGHT, 128);
    setLedBrightness(RMPLed::LCD_BRIGHTNESS, 128);
    setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 255);
    setAllLedsEnabled(false);

    setProfileForCurrentAircraft();

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(RMPLed::BACKLIGHT, 128);
                 setLedBrightness(RMPLed::LCD_BRIGHTNESS, 255);
                 setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);

                 AppState::getInstance()->executeAfter(2000, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
            {.name = "Variant", .content = std::vector<MenuItem>{
                                    {.name = "RMP1 (Captain)", .checked = deviceVariant == RMPDeviceVariant::VARIANT_CAPTAIN, .content = [this](int menuId) {
                                         setDeviceVariant(RMPDeviceVariant::VARIANT_CAPTAIN);
                                         PluginsMenu::getInstance()->uncheckSubmenuSiblings(menuId);
                                         PluginsMenu::getInstance()->setItemChecked(menuId, true);
                                     }},
                                    {.name = "RMP2 (First Officer)", .checked = deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER, .content = [this](int menuId) {
                                         setDeviceVariant(RMPDeviceVariant::VARIANT_FIRSTOFFICER);
                                         PluginsMenu::getInstance()->uncheckSubmenuSiblings(menuId);
                                         PluginsMenu::getInstance()->setItemChecked(menuId, true);
                                     }},
                                    {.name = "RMP3 (Stby/Center)", .checked = deviceVariant == RMPDeviceVariant::VARIANT_STBY, .content = [this](int menuId) {
                                         setDeviceVariant(RMPDeviceVariant::VARIANT_STBY);
                                         PluginsMenu::getInstance()->uncheckSubmenuSiblings(menuId);
                                         PluginsMenu::getInstance()->setItemChecked(menuId, true);
                                     }},
                                }},
        });

    return true;
}

void ProductRMP::blackout() {
    setLedBrightness(RMPLed::BACKLIGHT, 0);
    setLedBrightness(RMPLed::LCD_BRIGHTNESS, 0);
    setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 0);

    setAllLedsEnabled(false);
}

void ProductRMP::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (++displayUpdateFrameCounter >= getDisplayUpdateFrameInterval(12)) {
        displayUpdateFrameCounter = 0;
        updateDisplays(false);
    }
}

void ProductRMP::updateDisplays(bool force) {
    if (!connected || !profile) {
        return;
    }

    bool shouldUpdate = force;
    auto datarefManager = Dataref::getInstance();
    for (const std::string &dataref : profile->displayDatarefs()) {
        if (!lastUpdateCycle || datarefManager->getCachedLastUpdate(dataref.c_str()) > lastUpdateCycle) {
            shouldUpdate = true;
            break;
        }
    }

    if (!shouldUpdate) {
        return;
    }

    profile->updateDisplays();
    lastUpdateCycle = XPLMGetCycleNumber();
}

void ProductRMP::setDeviceVariant(RMPDeviceVariant variant) {
    if (deviceVariant == variant) {
        return;
    }

    if (profile) {
        delete profile;
        profile = nullptr;
        profileReady = false;
    }

    deviceVariant = variant;
    cachedActiveDisplay.clear();
    cachedStbyDisplay.clear();

    setProfileForCurrentAircraft();
    updateDisplays(true);
}

void ProductRMP::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(RMPLed::_START);
    unsigned char end = static_cast<unsigned char>(RMPLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        RMPLed led = static_cast<RMPLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductRMP::setLedBrightness(RMPLed led, uint8_t brightness) {
    int ledInt = static_cast<int>(led);
    auto it = lastLedBrightness.find(ledInt);
    if (it != lastLedBrightness.end() && it->second == brightness) {
        return;
    }
    lastLedBrightness[ledInt] = brightness;

    writeData({0x02, identifierByte, 0xBB, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductRMP::parseSegment(const std::string &text, int expectedLength, std::string &outDigits, uint16_t &colonMask, int digitOffset) {
    std::string digits;
    uint16_t localColonMask = 0;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        if (c == '.' && !digits.empty()) {
            localColonMask |= (1 << (digits.length() - 1));
        } else if (c == '/') {
            /* ToLiss datarefs use a C/course format for backup nav */
            digits += '-';
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

void ProductRMP::setDisplayText(const std::string &active, const std::string &stby) {
    if (active == cachedActiveDisplay && stby == cachedStbyDisplay) {
        return;
    }
    cachedActiveDisplay = active;
    cachedStbyDisplay = stby;

    std::vector<uint8_t> packet = {
        0xF0, 0x00, packetNumber, 0x35, identifierByte,
        0xBB, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    packet.resize(64, 0x00);

    const int byteOffset = 25;

    std::string allDigits;
    uint16_t colonMask = 0;

    /* Standby is first, Active is second */
    parseSegment(stby, 6, allDigits, colonMask, 0);
    parseSegment(active, 6, allDigits, colonMask, 6);

    for (int digitIndex = 0; digitIndex < 12; ++digitIndex) {
        char c = allDigits[digitIndex];
        uint8_t charMask = SegmentDisplay::getSegmentMask(c);

        if (colonMask & (1 << digitIndex)) {
            charMask |= 0x80;
        }

        packet[digitIndex + byteOffset] = charMask;
    }

    writeData(packet);

    std::vector<uint8_t> commitPacket = {
        0xF0, 0x00, packetNumber, 0x11, identifierByte,
        0xBB, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    commitPacket.resize(64, 0x00);
    writeData(commitPacket);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }
}

void ProductRMP::didReceiveData(int reportId, uint8_t *report, int reportLength) {
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

void ProductRMP::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    auto &buttons = profile->buttonDefs();
    auto it = buttons.find(hardwareButtonIndex);
    if (it == buttons.end()) {
        return;
    }

    const RMPButtonDef *buttonDef = &it->second;

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

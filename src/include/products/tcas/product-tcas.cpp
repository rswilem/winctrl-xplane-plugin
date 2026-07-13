#include "product-tcas.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/fps748-tcas-profile.h"
#include "profiles/pa28-tcas-profile.h"
#include "profiles/rotatemd11-tcas-profile.h"
#include "profiles/sparky744-tcas-profile.h"
#include "profiles/toliss-tcas-profile.h"
#include "profiles/xcrafts-erj-tcas-profile.h"
#include "profiles/zibo-tcas-profile.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

ProductTCAS::ProductTCAS(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    profile = nullptr;
    menuItemId = -1;
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductTCAS::~ProductTCAS() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductTCAS::classIdentifier() {
    return "TCAS";
}

const char *ProductTCAS::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductTCAS::setProfileForCurrentAircraft() {
    if (FPS748TCASProfile::IsEligible()) {
        profile = new FPS748TCASProfile(this);
        profileReady = true;
    } else if (RotateMD11TCASProfile::IsEligible()) {
        profile = new RotateMD11TCASProfile(this);
        profileReady = true;
    } else if (SparkyB744TCASProfile::IsEligible()) {
        profile = new SparkyB744TCASProfile(this);
        profileReady = true;
    } else if (XCraftsERJTCASProfile::IsEligible()) {
        profile = new XCraftsERJTCASProfile(this);
        profileReady = true;
    } else if (ZiboTCASProfile::IsEligible()) {
        profile = new ZiboTCASProfile(this);
        profileReady = true;
    } else if (TolissTCASProfile::IsEligible()) {
        profile = new TolissTCASProfile(this);
        profileReady = true;
    } else if (PA28TCASProfile::IsEligible()) {
        profile = new PA28TCASProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductTCAS::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(TCASLed::BACKLIGHT, 128);
    setLedBrightness(TCASLed::LCD_BRIGHTNESS, 128);
    setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, 255);
    setAllLedsEnabled(false);

    setProfileForCurrentAircraft();

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(TCASLed::BACKLIGHT, 128);
                 setLedBrightness(TCASLed::LCD_BRIGHTNESS, 255);
                 setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);

                 AppState::getInstance()->executeAfter(2000, this, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
        });

    return true;
}

void ProductTCAS::blackout() {
    setLedBrightness(TCASLed::BACKLIGHT, 0);
    setLedBrightness(TCASLed::LCD_BRIGHTNESS, 0);
    setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, 0);

    setAllLedsEnabled(false);
}

void ProductTCAS::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (++displayUpdateFrameCounter >= getDisplayUpdateFrameInterval(12)) {
        displayUpdateFrameCounter = 0;
        updateDisplays();
    }
}

void ProductTCAS::updateDisplays(bool force) {
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

void ProductTCAS::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(TCASLed::_START);
    unsigned char end = static_cast<unsigned char>(TCASLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        TCASLed led = static_cast<TCASLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductTCAS::setLedBrightness(TCASLed led, uint8_t brightness) {
    writeData({0x02, ProductTCAS::IdentifierByte, 0xBB, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductTCAS::setLCDText(const std::string &squawkCode) {
    std::vector<uint8_t> packet = {
        0xF0, 0x00, packetNumber, 0x35, ProductTCAS::IdentifierByte,
        0xBB, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    packet.resize(64, 0x00);

    const int rowOffsets[7] = {37, 33, 29, 25, 49, 45, 41};

    std::string allDigits = squawkCode;

    for (int digitIndex = 0; digitIndex < 4; ++digitIndex) {
        if (digitIndex < static_cast<int>(allDigits.length())) {
            char c = allDigits[digitIndex];
            uint8_t charMask = SegmentDisplay::getSegmentMask(c);

            for (int segIndex = 0; segIndex < 7; ++segIndex) {
                if (charMask & (1 << segIndex)) {
                    int byteOffset = rowOffsets[segIndex] + (digitIndex / 8);
                    int bitPos = digitIndex % 8;
                    packet[byteOffset] |= (1 << bitPos);
                }
            }
        }
    }

    writeData(packet);

    std::vector<uint8_t> commitPacket = {
        0xF0, 0x00, packetNumber, 0x11, ProductTCAS::IdentifierByte,
        0xBB, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    commitPacket.resize(64, 0x00);
    writeData(commitPacket);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }
}

void ProductTCAS::didReceiveData(int reportId, uint8_t *report, int reportLength) {
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

void ProductTCAS::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
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

    const TCASButtonDef *buttonDef = &it->second;

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

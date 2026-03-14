#include "product-ursa-minor-throttle.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/ff777-ursa-minor-throttle-profile.h"
#include "profiles/toliss-ursa-minor-throttle-profile.h"
#include "profiles/zibo-ursa-minor-throttle-profile.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

ProductUrsaMinorThrottle::ProductUrsaMinorThrottle(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;
    pressedButtonIndices = {};

    connect();
}

ProductUrsaMinorThrottle::~ProductUrsaMinorThrottle() {
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductUrsaMinorThrottle::classIdentifier() {
    return "Ursa Minor Throttle";
}

void ProductUrsaMinorThrottle::setProfileForCurrentAircraft() {
    if (TolissUrsaMinorThrottleProfile::IsEligible()) {
        profile = new TolissUrsaMinorThrottleProfile(this);
        profileReady = true;
    } else if (FF777UrsaMinorThrottleProfile::IsEligible()) {
        profile = new FF777UrsaMinorThrottleProfile(this);
        profileReady = true;
    } else if (ZiboUrsaMinorThrottleProfile::IsEligible()) {
        profile = new ZiboUrsaMinorThrottleProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductUrsaMinorThrottle::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, 128);
    setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, 255);
    setAllLedsEnabled(false);

    setProfileForCurrentAircraft();

    std::string vibrationPreference = AppState::getInstance()->readPreference("ThrottleVibration", "disabled"); // For the throttle, we disable vibration by default.
    loadVibrationSetting(vibrationPreference);

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, 128);
                 setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, 255);
                 setAllLedsEnabled(true);
                 AppState::getInstance()->executeAfter(2000, [this]() {
                     setAllLedsEnabled(false);
                 });
             }},
            {.name = "Vibration", .content = std::vector<MenuItem>{

                                      {.name = "Disabled", .checked = vibrationPreference == "disabled", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("ThrottleVibration", "disabled");
                                           loadVibrationSetting("disabled");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                      {.name = "Normal", .checked = vibrationPreference == "normal", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("ThrottleVibration", "normal");
                                           loadVibrationSetting("normal");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                      {.name = "Strong", .checked = vibrationPreference == "strong", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("ThrottleVibration", "strong");
                                           loadVibrationSetting("strong");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                  }},
        });

    return true;
}

void ProductUrsaMinorThrottle::blackout() {
    setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, 0);
    setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, 0);

    setAllLedsEnabled(false);
}

void ProductUrsaMinorThrottle::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on")) {
        float gForce = Dataref::getInstance()->get<float>("sim/flightmodel/forces/g_nrml");
        float delta = fabs(gForce - lastGForce);
        lastGForce = gForce;

        bool onGround = Dataref::getInstance()->getCached<bool>("sim/flightmodel/failures/onground_any");
        uint8_t vibration = (uint8_t) std::min(255.0f, delta * vibrationMultiplier / (onGround ? 1.0f : 2.0f));
        if (vibration < 6) {
            vibration = 0;
        }

        if (lastVibration != vibration) {
            setVibration(vibration, true, true);
            lastVibration = vibration;
        }
    } else if (lastVibration > 0) {
        lastVibration = 0;
        setVibration(lastVibration, true, true);
    }

    if (profile) {
        profile->update();

        // We explicity do not want an auto update, we simply listen for the dataref and only then update.
        // profile->updateDisplays();
    }
}

void ProductUrsaMinorThrottle::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(UrsaMinorThrottleLed::_START);
    unsigned char end = static_cast<unsigned char>(UrsaMinorThrottleLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        UrsaMinorThrottleLed led = static_cast<UrsaMinorThrottleLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductUrsaMinorThrottle::setLedBrightness(UrsaMinorThrottleLed led, uint8_t brightness) {
    writeData({0x02, ProductUrsaMinorThrottle::ThrottleIdentifierByte, 0xB9, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});

    if (led < UrsaMinorThrottleLed::_START) {
        writeData({0x02, ProductUrsaMinorThrottle::PACIdentifierByte, 0xB9, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
    }
}

void ProductUrsaMinorThrottle::setVibration(uint8_t vibration, bool leftSide, bool rightSide) {
    if (vibrationMultiplier <= std::numeric_limits<float>::epsilon()) {
        return;
    }

    if (leftSide) {
        writeData({0x02, ProductUrsaMinorThrottle::ThrottleIdentifierByte, 0xB9, 0x00, 0x00, 0x03, 0x49, 0x0E, vibration, 0x00, 0x00, 0x00, 0x00, 0x00});
    }

    if (rightSide) {
        writeData({0x02, ProductUrsaMinorThrottle::ThrottleIdentifierByte, 0xB9, 0x00, 0x00, 0x03, 0x49, 0x10, vibration, 0x00, 0x00, 0x00, 0x00, 0x00});
    }
}

void ProductUrsaMinorThrottle::setLCDText(const std::string &text) {
    bool unknownFlag = false;
    std::vector<uint8_t> packet = {
        0xF0, 0x00, packetNumber, 0x35, ProductUrsaMinorThrottle::PACIdentifierByte, 0xB9,
        0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
        0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    packet.resize(64, 0x00);

    const int rowOffsets[9] = {53, 49, 45, 41, 37, 33, 29, 25, 57};
    const int digitBits[4] = {0, 1, 2, 3};

    std::string charsOnly = "";
    uint8_t dotsMask = 0;

    for (char c : text) {
        if (c == '.') {
            if (!charsOnly.empty()) {
                dotsMask |= (1 << (charsOnly.length() - 1));
            }
        } else if (c == 'R') {
            charsOnly += 'A'; // Represent 'R' as 'A'
        } else {
            charsOnly += c;
        }
    }

    while (charsOnly.length() < 4) {
        charsOnly += " ";
    }
    if (charsOnly.length() > 4) {
        charsOnly = charsOnly.substr(0, 4);
    }

    if (charsOnly.length() == 3 && !isdigit(charsOnly[0])) {
        charsOnly.insert(1, " ");
        uint8_t maskAfterFirst = dotsMask & ~1;
        uint8_t maskFirst = dotsMask & 1;
        dotsMask = maskFirst | (maskAfterFirst << 1);
    }

    if (charsOnly.length() > 4) {
        charsOnly = charsOnly.substr(0, 4);
    }
    while (charsOnly.length() < 4) {
        charsOnly += " ";
    }

    for (int i = 0; i < 4; ++i) {
        char c = charsOnly[i];
        int targetBit = digitBits[i];

        uint16_t mask = SegmentDisplay::getSegmentMask(c);

        // Apply the unknown flag ONLY to the first character (L or R)
        if (i == 0 && unknownFlag) {
            mask |= 0x100; // Set Bit 8
        }

        for (int segIndex = 0; segIndex < 9; ++segIndex) {
            bool turnOn = false;

            if (segIndex == 7) {
                if ((dotsMask >> i) & 1) {
                    turnOn = true;
                }
            } else {
                if ((mask >> segIndex) & 1) {
                    turnOn = true;
                }
            }

            if (turnOn) {
                int byteOffset = rowOffsets[segIndex];
                packet[byteOffset] |= (1 << targetBit);
            }
        }
    }

    writeData(packet);

    std::vector<uint8_t> commitPacket = {
        0xF0, 0x00, packetNumber, 0x11, 0x01, 0xB9,
        0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    commitPacket.resize(64, 0x00);
    writeData(commitPacket);

    if (++packetNumber == 0) {
        packetNumber = 1;
    }
}

void ProductUrsaMinorThrottle::forceStateSync() {
    pressedButtonIndices.clear();
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;

    USBDevice::forceStateSync();
}

void ProductUrsaMinorThrottle::didReceiveData(int reportId, uint8_t *report, int reportLength) {
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

void ProductUrsaMinorThrottle::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    auto &buttons = profile->buttonDefs();
    auto it = buttons.find(hardwareButtonIndex);
    if (it == buttons.end()) {
        return;
    }

    const UrsaMinorThrottleButtonDef *buttonDef = &it->second;
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

void ProductUrsaMinorThrottle::loadVibrationSetting(const std::string &preference) {
    if (preference == "disabled") {
        vibrationMultiplier = 1.0f;
        setVibration(0);
        vibrationMultiplier = 0.0f;
    } else if (preference == "strong") {
        vibrationMultiplier = 900.0f;
    } else {
        vibrationMultiplier = 600.0f;
    }
}

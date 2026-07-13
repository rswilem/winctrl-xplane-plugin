#include "pa28-tcas-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-tcas.h"
#include "segment-display.h"
#include "logger.hpp"

#include <algorithm>
#include <cmath>

PA28TCASProfile::PA28TCASProfile(ProductTCAS *product) : TCASAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio", [this, product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        float panel = 0.0f;
        for (size_t i = 0; i < brightness.size() && i < 4; i++) {
            panel = std::max(panel, brightness[i]);
        }

        uint8_t backlight = hasPower ? panel * 255 : 0;
        uint8_t screens = hasPower ? 255 : 0;

        if (backlight == lastBacklightSent && screens == lastScreensSent) {
            return;
        }
        lastBacklightSent = backlight;
        lastScreensSent = screens;

        product->setLedBrightness(TCASLed::BACKLIGHT, backlight);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, screens);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Logger::getInstance()->info("TCAS: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28TCASProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::vector<std::string> &PA28TCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit2/radios/actuators/transponder_code",
        "sim/cockpit2/radios/actuators/transponder_mode",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &PA28TCASProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, TCASButtonDef> buttons = {
        {0, {"Keypad 1", "custom_digit", TCASDatarefType::SET_VALUE, 1}},
        {1, {"Keypad 2", "custom_digit", TCASDatarefType::SET_VALUE, 2}},
        {2, {"Keypad 3", "custom_digit", TCASDatarefType::SET_VALUE, 3}},
        {3, {"Keypad 4", "custom_digit", TCASDatarefType::SET_VALUE, 4}},
        {4, {"Keypad 5", "custom_digit", TCASDatarefType::SET_VALUE, 5}},
        {5, {"Keypad 6", "custom_digit", TCASDatarefType::SET_VALUE, 6}},
        {6, {"Keypad 7", "custom_digit", TCASDatarefType::SET_VALUE, 7}},
        {7, {"Keypad 0", "custom_digit", TCASDatarefType::SET_VALUE, 0}},
        {8, {"Keypad CLR", "custom_clr", TCASDatarefType::SET_VALUE, 0}},
        {9, {"Ident", "sim/transponder/transponder_ident", TCASDatarefType::EXECUTE_CMD_PHASED}},

        // Map the Airbus mode selector onto the PA28 transponder: 1 = STBY, 2 = ON (Mode A), 3 = ALT (Mode C)
        {10, {"XPDR STBY", "sim/cockpit2/radios/actuators/transponder_mode", TCASDatarefType::SET_VALUE, 1}},
        {11, {"XPDR AUTO", "sim/cockpit2/radios/actuators/transponder_mode", TCASDatarefType::SET_VALUE, 3}},
        {12, {"XPDR ON", "sim/cockpit2/radios/actuators/transponder_mode", TCASDatarefType::SET_VALUE, 2}},
        {15, {"ALT RPTG OFF", "sim/cockpit2/radios/actuators/transponder_mode", TCASDatarefType::SET_VALUE, 2}},
        {16, {"ALT RPTG ON", "sim/cockpit2/radios/actuators/transponder_mode", TCASDatarefType::SET_VALUE, 3}},
    };

    return buttons;
}

void PA28TCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->dataref == "custom_digit") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        codeEntry += std::to_string(static_cast<int>(button->value));
        if (codeEntry.length() >= 4) {
            datarefManager->set<int>("sim/cockpit2/radios/actuators/transponder_code", std::stoi(codeEntry));
            codeEntry = "";
        }

        product->updateDisplays(true);
    } else if (button->dataref == "custom_clr") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        if (!codeEntry.empty()) {
            codeEntry.pop_back();
        }

        product->updateDisplays(true);
    } else if (button->datarefType == TCASDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void PA28TCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    bool hasPower = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    int mode = datarefManager->getCached<int>("sim/cockpit2/radios/actuators/transponder_mode");

    std::string squawkCode = "";
    if (hasPower && mode > 0) {
        if (!codeEntry.empty()) {
            squawkCode = codeEntry;
        } else {
            int code = datarefManager->getCached<int>("sim/cockpit2/radios/actuators/transponder_code");
            squawkCode = SegmentDisplay::fixStringLength(std::to_string(code), 4);
        }
    }

    product->setLCDText(squawkCode);
}

#include "zibo-tcas-profile.h"

#include "dataref.h"
#include "product-tcas.h"

#include <cstdio>
#include <string>
#include <XPLMUtilities.h>

ZiboTCASProfile::ZiboTCASProfile(ProductTCAS *product) : TCASAircraftProfile(product), squawkInput("") {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/panel_brightness_ratio", [product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/battery_on");
        uint8_t backlight = (hasPower && !brightness.empty()) ? static_cast<uint8_t>(brightness[0] * 255) : 0;
        product->setLedBrightness(TCASLed::BACKLIGHT, backlight);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/panel_brightness_ratio");
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/battery_on");
}

bool ZiboTCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::vector<std::string> &ZiboTCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/radios/actuators/transponder_code",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &ZiboTCASProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, TCASButtonDef> buttons = {
        // Keypad
        {0, {"Keypad 1", "set_keypad"}},
        {1, {"Keypad 2", "set_keypad"}},
        {2, {"Keypad 3", "set_keypad"}},
        {3, {"Keypad 4", "set_keypad"}},
        {4, {"Keypad 5", "set_keypad"}},
        {5, {"Keypad 6", "set_keypad"}},
        {6, {"Keypad 7", "set_keypad"}},
        {7, {"Keypad 0", "set_keypad"}},
        {8, {"Keypad CLR", "clear_keypad"}},

        // Ident
        {9, {"Ident", "laminar/B738/push_button/transponder_ident_dn", TCASDatarefType::EXECUTE_CMD_PHASED}},

        // Transponder mode selector
        {10, {"XPDR STBY", "laminar/B738/knob/transponder_stby", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"XPDR ON", "laminar/B738/knob/transponder_alton", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"ALT RPTG OFF", "laminar/B738/knob/transponder_altoff", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {22, {"TCAS TA", "laminar/B738/knob/transponder_ta", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {23, {"TCAS TA/RA", "laminar/B738/knob/transponder_tara", TCASDatarefType::EXECUTE_CMD_PHASED}},
    };
    return buttons;
}

void ZiboTCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (button->dataref == "set_keypad") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        char digit = button->name.back();
        if (squawkInput.length() < 4) {
            squawkInput += digit;
        }
        if (squawkInput.length() == 4) {
            int code = std::stoi(squawkInput);
            dm->set<int>("sim/cockpit2/radios/actuators/transponder_code", code);
        }
    } else if (button->dataref == "clear_keypad") {
        if (phase == xplm_CommandBegin) {
            squawkInput.clear();
        }
    } else {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboTCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string code;
    if (squawkInput.empty()) {
        int transpCode = Dataref::getInstance()->getCached<int>("sim/cockpit2/radios/actuators/transponder_code");
        char buf[5];
        snprintf(buf, sizeof(buf), "%04d", transpCode);
        code = std::string(buf);
    } else {
        code = squawkInput;
        code.append(4 - code.length(), '-');
    }

    product->setLCDText(code);
}

#include "xcrafts-erj-tcas-profile.h"

#include "dataref.h"
#include "product-tcas.h"

#include <cstdio>
#include <string>
#include <XPLMUtilities.h>

XCraftsERJTCASProfile::XCraftsERJTCASProfile(ProductTCAS *product) : TCASAircraftProfile(product), squawkInput("") {
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

XCraftsERJTCASProfile::~XCraftsERJTCASProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool XCraftsERJTCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/ERJ/MFD1/WX_TERR_status");
}

const std::vector<std::string> &XCraftsERJTCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/radios/transponder_code",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &XCraftsERJTCASProfile::buttonDefs() const {
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
        {9, {"Ident", "sim/transponder/transponder_ident", TCASDatarefType::EXECUTE_CMD_PHASED}},

        // Transponder mode selector
        {10, {"XPDR STBY", "sim/cockpit/radios/transponder_mode", TCASDatarefType::SET_VALUE, 1}},
        {11, {"", ""}},
        {12, {"XPDR ON", "sim/cockpit/radios/transponder_mode", TCASDatarefType::SET_VALUE, 2}},
        {13, {"", ""}},
        {14, {"", ""}},
        {15, {"ALT RPTG OFF", ""}},
        {16, {"ALT RPTG ON", ""}},
        {17, {"TCAS TFC THRT", ""}},
        {18, {"TCAS TFC ALL", ""}},
        {19, {"TCAS TFC ABV", ""}},
        {20, {"TCAS TFC BLW", ""}},
        {21, {"TCAS MODE STBY", "sim/cockpit/radios/transponder_mode", TCASDatarefType::SET_VALUE, 1}},
        {22, {"TCAS TA", "sim/cockpit/radios/transponder_mode", TCASDatarefType::SET_VALUE, 6}},
        {23, {"TCAS TA/RA", "sim/cockpit/radios/transponder_mode", TCASDatarefType::SET_VALUE, 7}},
    };
    return buttons;
}

void XCraftsERJTCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
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
            dm->set<int>("sim/cockpit/radios/transponder_code", code);
        }
    } else if (button->dataref == "clear_keypad") {
        if (phase == xplm_CommandBegin) {
            squawkInput.clear();
        }
    } else if (button->datarefType == TCASDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        dm->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

void XCraftsERJTCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string code;
    if (squawkInput.empty()) {
        int transpCode = Dataref::getInstance()->get<int>("sim/cockpit/radios/transponder_code");
        char buf[5];
        snprintf(buf, sizeof(buf), "%04d", transpCode);
        code = std::string(buf);
    } else {
        code = squawkInput;
        code.append(4 - code.length(), '-');
    }

    product->setLCDText(code);
}

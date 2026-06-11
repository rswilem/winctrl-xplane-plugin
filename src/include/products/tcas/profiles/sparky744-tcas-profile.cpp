#include "sparky744-tcas-profile.h"

#include "dataref.h"
#include "product-tcas.h"

#include <string>
#include <XPLMUtilities.h>

SparkyB744TCASProfile::SparkyB744TCASProfile(ProductTCAS *product) : TCASAircraftProfile(product), squawkInput("") {
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool powered) {
        uint8_t backlight = powered ? 200 : 0;
        product->setLedBrightness(TCASLed::BACKLIGHT, backlight);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, powered ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, powered ? 255 : 0);
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
}

SparkyB744TCASProfile::~SparkyB744TCASProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool SparkyB744TCASProfile::IsEligible() {
    auto dr = Dataref::getInstance();
    return dr->exists("laminar/B747/fms1/Line01_L")
        && !dr->exists("FPS/748/simtime")
        && !dr->exists("SSG/748/simtime");
}

const std::vector<std::string> &SparkyB744TCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/radios/transponder_code",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &SparkyB744TCASProfile::buttonDefs() const {
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
        {9, {"Ident", "laminar/B747/flt_mgmt/transponder/ident_btn_pos", TCASDatarefType::SET_VALUE_PHASED, 1}},

        // Transponder mode selector dial
        {10, {"XPDR STBY", "laminar/B747/flt_mgmt/transponder/sel_dial", TCASDatarefType::SET_VALUE, 0}},
        {12, {"XPDR ON", "laminar/B747/flt_mgmt/transponder/sel_dial", TCASDatarefType::SET_VALUE, 1}},
        {15, {"ALT RPTG OFF", "laminar/B747/flt_mgmt/transponder/sel_dial", TCASDatarefType::SET_VALUE, 2}},
        {22, {"TCAS TA", "laminar/B747/flt_mgmt/transponder/sel_dial", TCASDatarefType::SET_VALUE, 3}},
        {23, {"TCAS TA/RA", "laminar/B747/flt_mgmt/transponder/sel_dial", TCASDatarefType::SET_VALUE, 4}},

        // Transponder mode dial up/down
        {20, {"MODE UP", "laminar/B747/flt_mgmt/transponder/mode_sel_dial_up"}},
        {21, {"MODE DN", "laminar/B747/flt_mgmt/transponder/mode_sel_dial_dn"}},
    };
    return buttons;
}

void SparkyB744TCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
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
    } else if (!button->dataref.empty()) {
        if (button->datarefType == TCASDatarefType::SET_VALUE_PHASED) {
            dm->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? static_cast<int>(button->value) : 0);
        } else if (button->datarefType == TCASDatarefType::SET_VALUE) {
            if (phase == xplm_CommandBegin) {
                dm->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
            }
        } else {
            dm->executeCommand(button->dataref.c_str(), phase);
        }
    }
}

void SparkyB744TCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string code;
    if (squawkInput.empty()) {
        auto dm = Dataref::getInstance();
        int transpCode = dm->getCached<int>("sim/cockpit/radios/transponder_code");

        code.reserve(4);
        code += static_cast<char>('0' + ((transpCode / 1000) % 10));
        code += static_cast<char>('0' + ((transpCode / 100) % 10));
        code += static_cast<char>('0' + ((transpCode / 10) % 10));
        code += static_cast<char>('0' + (transpCode % 10));
    } else {
        code = squawkInput;
        code.append(4 - code.length(), '-');
    }

    product->setLCDText(code);
}

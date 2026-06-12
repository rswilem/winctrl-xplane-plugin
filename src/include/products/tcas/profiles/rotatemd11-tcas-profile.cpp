#include "rotatemd11-tcas-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-tcas.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

RotateMD11TCASProfile::RotateMD11TCASProfile(ProductTCAS *product) : TCASAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd", [product](bool hasPower) {
        float panelBrt = hasPower ? Dataref::getInstance()->getCached<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio") : 0.0f;
        uint8_t backlight = hasPower ? static_cast<uint8_t>(std::clamp(panelBrt, 0.0f, 1.0f) * 255) : 0;

        product->setLedBrightness(TCASLed::BACKLIGHT, backlight);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio", [](float) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    }, this);
}

bool RotateMD11TCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/systems/gcp_alt_presel_ft");
}

const std::vector<std::string> &RotateMD11TCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "Rotate/aircraft/systems/atc_active_code",
        "Rotate/aircraft/systems/elec_dc_batt_bus_pwrd",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &RotateMD11TCASProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, TCASButtonDef> buttons = {
        {0, {"Keypad 1", "Rotate/aircraft/controls_c/atc_pad_1"}},
        {1, {"Keypad 2", "Rotate/aircraft/controls_c/atc_pad_2"}},
        {2, {"Keypad 3", "Rotate/aircraft/controls_c/atc_pad_3"}},
        {3, {"Keypad 4", "Rotate/aircraft/controls_c/atc_pad_4"}},
        {4, {"Keypad 5", "Rotate/aircraft/controls_c/atc_pad_5"}},
        {5, {"Keypad 6", "Rotate/aircraft/controls_c/atc_pad_6"}},
        {6, {"Keypad 7", "Rotate/aircraft/controls_c/atc_pad_7"}},
        {7, {"Keypad 0", "Rotate/aircraft/controls_c/atc_pad_0"}},
        {8, {"Keypad CLR", "Rotate/aircraft/controls_c/atc_pad_clr"}},
        {9, {"Ident", "Rotate/aircraft/controls_c/atc_ident"}},
        {10, {"XPDR STBY", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 0}},
        {11, {"XPDR ALT RPTG OFF", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 1}},
        {12, {"XPDR ON", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 2}},
        {13, {"XPDR Unit 1", ""}},
        {14, {"XPDR Unit 2", ""}},
        {15, {"ALT RPTG OFF", "Rotate/aircraft/controls/atc_alt_rpt", TCASDatarefType::SET_VALUE, 0}},
        {16, {"ALT RPTG ON", "Rotate/aircraft/controls/atc_alt_rpt", TCASDatarefType::SET_VALUE, 1}},
        {17, {"TCAS TFC THRT", ""}},
        {18, {"TCAS TFC ALL", ""}},
        {19, {"TCAS TFC ABV", ""}},
        {20, {"TCAS TFC BLW", ""}},
        {21, {"TCAS MODE STBY", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 0}},
        {22, {"TCAS MODE TA", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 3}},
        {23, {"TCAS MODE TA/RA", "Rotate/aircraft/controls/atc_mode_sel", TCASDatarefType::SET_VALUE, 4}},
    };
    return buttons;
}

void RotateMD11TCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == TCASDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }
        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void RotateMD11TCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    bool hasPower = Dataref::getInstance()->getCached<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    if (!hasPower) {
        product->setLCDText("");
        return;
    }

    auto code = Dataref::getInstance()->getCached<std::vector<int>>("Rotate/aircraft/systems/atc_active_code");
    if (code.empty()) {
        product->setLCDText("0000");
        return;
    }

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(4) << code[0];
    product->setLCDText(ss.str());
}

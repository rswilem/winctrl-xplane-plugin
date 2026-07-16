#include "toliss-tcas-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-tcas.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

TolissTCASProfile::TolissTCASProfile(ProductTCAS *product) : TCASAircraftProfile(product) {
    buttons = {
        {0, {"Keypad 1", "AirbusFBW/ATCCodeKey1", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {1, {"Keypad 2", "AirbusFBW/ATCCodeKey2", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"Keypad 3", "AirbusFBW/ATCCodeKey3", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"Keypad 4", "AirbusFBW/ATCCodeKey4", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"Keypad 5", "AirbusFBW/ATCCodeKey5", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"Keypad 6", "AirbusFBW/ATCCodeKey6", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"Keypad 7", "AirbusFBW/ATCCodeKey7", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"Keypad 0", "AirbusFBW/ATCCodeKey0", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"Keypad CLR", "AirbusFBW/ATCCodeKeyCLR", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"Ident", "sim/transponder/transponder_ident", TCASDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"XPDR STBY", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 0}},
        {11, {"XPDR AUTO", "AirbusFBW/XPDRTCASMode", TCASDatarefType::SET_VALUE, 0}},
        {12, {"XPDR ON", "AirbusFBW/XPDRTCASMode", TCASDatarefType::SET_VALUE, 1}},
        {13, {"XPDR Unit 1", "AirbusFBW/XPDRSystem", TCASDatarefType::SET_VALUE, 1}},
        {14, {"XPDR Unit 2", "AirbusFBW/XPDRSystem", TCASDatarefType::SET_VALUE, 2}},
        {15, {"ALT RPTG OFF", ""}},
        {16, {"ALT RPTG ON", ""}},
        {17, {"TCAS TFC THRT", ""}},
        {18, {"TCAS TFC ALL", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, 1}},
        {19, {"TCAS TFC ABV", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, 0}},
        {20, {"TCAS TFC BLW", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, 2}},
        {21, {"TCAS MODE STBY", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 0}},
        {22, {"TCAS MODE TA", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 3}},
        {23, {"TCAS MODE TA/RA", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 4}},
    };

    std::string icao = Dataref::getInstance()->getCached<std::string>("sim/aircraft/view/acf_ICAO");
    if (icao == "A339" || icao == "A346") {
        buttons[11] = {"XPDR AUTO", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 1};
        buttons[12] = {"XPDR ON", "AirbusFBW/XPDRPower", TCASDatarefType::SET_VALUE, 2};
        buttons[15] = {"ALT RPTG OFF", "AirbusFBW/XPDRAltitude", TCASDatarefType::SET_VALUE, 0};
        buttons[16] = {"ALT RPTG ON", "AirbusFBW/XPDRAltitude", TCASDatarefType::SET_VALUE, 1};
        buttons[17] = {"TCAS TFC THRT", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, -1};
        buttons[18] = {"TCAS TFC ALL", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, 0};
        buttons[19] = {"TCAS TFC ABV", "AirbusFBW/XPDRTCASAltSelect", TCASDatarefType::SET_VALUE, 1};
        buttons[21] = {"TCAS MODE STBY", "AirbusFBW/XPDRTCASMode", TCASDatarefType::SET_VALUE, 0};
        buttons[22] = {"TCAS MODE TA", "AirbusFBW/XPDRTCASMode", TCASDatarefType::SET_VALUE, 1};
        buttons[23] = {"TCAS MODE TA/RA", "AirbusFBW/XPDRTCASMode", TCASDatarefType::SET_VALUE, 2};
    }

    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasEssentialBusPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(TCASLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this, product](int annunMode) {
        // Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/OHPLightsATA32_Raw");

        product->setLedBrightness(TCASLed::ATC_FAIL, isAnnunTest() ? 255 : 0);
        updateDisplays();
    },
        this);

    // Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/OHPLightsATA32_Raw", [this, product](const std::vector<float> &panelLights) {
    //     if (panelLights.size() < 18) {
    //         return;
    //     }
    //     product->setLedBrightness(TCASLed::ATC_FAIL, panelLights[0] > std::numeric_limits<float>::epsilon() || isAnnunTest() ? 1 : 0);
    // });
}

bool TolissTCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::vector<std::string> &TolissTCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "AirbusFBW/XPDRString",
        "AirbusFBW/AnnunMode",
        "sim/cockpit/electrical/avionics_on",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &TolissTCASProfile::buttonDefs() const {
    return buttons;
}

void TolissTCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
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

void TolissTCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string squawkCode = isAnnunTest() ? "8888" : Dataref::getInstance()->getCached<std::string>("AirbusFBW/XPDRString");
    product->setLCDText(squawkCode);
}

bool TolissTCASProfile::isAnnunTest() {
    return Dataref::getInstance()->getCached<int>("AirbusFBW/AnnunMode") == 2 && Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on");
}

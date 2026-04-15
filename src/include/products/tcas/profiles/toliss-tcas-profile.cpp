#include "toliss-tcas-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-tcas.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

TolissTCASProfile::TolissTCASProfile(ProductTCAS *product) : TCASAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasEssentialBusPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(TCASLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    });

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this, product](int annunMode) {
        // Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/OHPLightsATA32_Raw");

        product->setLedBrightness(TCASLed::ATC_FAIL, isAnnunTest() ? 255 : 0);
        updateDisplays();
    });

    // Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/OHPLightsATA32_Raw", [this, product](const std::vector<float> &panelLights) {
    //     if (panelLights.size() < 18) {
    //         return;
    //     }
    //     product->setLedBrightness(TCASLed::ATC_FAIL, panelLights[0] || isAnnunTest() ? 1 : 0);
    // });
}

TolissTCASProfile::~TolissTCASProfile() {
    Dataref::getInstance()->unbind("AirbusFBW/PanelBrightnessLevel");
    Dataref::getInstance()->unbind("AirbusFBW/FCUAvail");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("AirbusFBW/OHPLightsATA32_Raw");
}

bool TolissTCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::unordered_map<uint16_t, TCASButtonDef> &TolissTCASProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, TCASButtonDef> buttons = {
        {0, {"Keypad 1", ""}},
        {1, {"Keypad 2", ""}},
        {2, {"Keypad 3", ""}},
        {3, {"Keypad 4", ""}},
        {4, {"Keypad 5", ""}},
        {5, {"Keypad 6", ""}},
        {6, {"Keypad 7", ""}},
        {7, {"Keypad 0", ""}},
        {8, {"Keypad CLR", ""}},
        {9, {"Ident", ""}},
        {10, {"XPDR STBY", ""}},
        {11, {"XPDR AUTO", ""}},
        {12, {"XPDR ON", ""}},
        {13, {"XPDR Unit 1", ""}},
        {14, {"XPDR Unit 2", ""}},
        {15, {"ALT RPTG OFF", ""}},
        {16, {"ALT RPTG ON", ""}},
        {17, {"TCAS TFC THRT", ""}},
        {18, {"TCAS TFC ALL", ""}},
        {19, {"TCAS TFC ABV", ""}},
        {20, {"TCAS TFC BLW", ""}},
        {21, {"TCAS MODE STBY", ""}},
        {22, {"TCAS MODE TA", ""}},
        {23, {"TCAS MODE TA/RA", ""}},
    };

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

    std::string squawkCode = "1234";
    if (isAnnunTest()) {
        squawkCode = "8888";
    }

    product->setLCDText(squawkCode);
}

bool TolissTCASProfile::isAnnunTest() {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2 && Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
}

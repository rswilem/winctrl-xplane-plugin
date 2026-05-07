#include "toliss-ecam-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ecam.h"
#include "xplane-version.hpp"

#include <algorithm>
#include <cmath>

TolissECAMProfile::TolissECAMProfile(ProductECAM *product) : ECAMAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        bool ecpAvailable = Dataref::getInstance()->get<bool>("AirbusFBW/ECPAvail");
        uint8_t backlightBrightness = hasPower && ecpAvailable ? brightness * 255 : 0;

        product->setLedBrightness(ECAMLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(ECAMLed::EMER_CANC_BRIGHTNESS, backlightBrightness);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref(ifXPlane11("AirbusFBW/OHPLightsATA31", "AirbusFBW/OHPLightsATA31_Raw"));
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ECPAvail", [this, product](bool enabled) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref(ifXPlane11("AirbusFBW/OHPLightsATA31", "AirbusFBW/OHPLightsATA31_Raw"));
    });

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(ifXPlane11("AirbusFBW/OHPLightsATA31", "AirbusFBW/OHPLightsATA31_Raw"), [product](const std::vector<float> &panelLights) {
        if (panelLights.size() < 45) {
            return;
        }

        product->setLedBrightness(ECAMLed::ENG, panelLights[30] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::BLEED, panelLights[31] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::PRESS, panelLights[32] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::ELEC, panelLights[33] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::HYD, panelLights[34] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::FUEL, panelLights[35] > std::numeric_limits<float>::epsilon() ? 1 : 0);

        product->setLedBrightness(ECAMLed::APU, panelLights[36] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::COND, panelLights[37] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::DOOR, panelLights[38] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::WHEEL, panelLights[39] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::F_CTL, panelLights[40] > std::numeric_limits<float>::epsilon() ? 1 : 0);

        product->setLedBrightness(ECAMLed::STS, panelLights[41] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::CLR_LEFT, panelLights[42] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(ECAMLed::CLR_RIGHT, panelLights[43] > std::numeric_limits<float>::epsilon() ? 1 : 0);
    });
}

TolissECAMProfile::~TolissECAMProfile() {
    Dataref::getInstance()->unbind("AirbusFBW/PanelBrightnessLevel");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("AirbusFBW/ECPAvail");
    Dataref::getInstance()->unbind(ifXPlane11("AirbusFBW/OHPLightsATA31", "AirbusFBW/OHPLightsATA31_Raw"));
}

bool TolissECAMProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::unordered_map<uint16_t, ECAMButtonDef> &TolissECAMProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, ECAMButtonDef> buttons = {
        {0, {"EMPTY 1", ""}},
        {1, {"TOCONFIG", "AirbusFBW/TOConfigPress"}},
        {2, {"EMPTY 2", ""}},
        {3, {"EMER", "AirbusFBW/EmerCancel"}},
        {4, {"ENG", "AirbusFBW/ECP/SelectEnginePage"}},
        {5, {"BLEED", "AirbusFBW/ECP/SelectBleedPage"}},
        {6, {"PRESS", "AirbusFBW/ECP/SelectPressPage"}},
        {7, {"ELEC", "AirbusFBW/ECP/SelectElecACPage"}},
        {8, {"HYD", "AirbusFBW/ECP/SelectHydraulicPage"}},
        {9, {"FUEL", "AirbusFBW/ECP/SelectFuelPage"}},
        {10, {"APU", "AirbusFBW/ECP/SelectAPUPage"}},
        {11, {"COND", "AirbusFBW/ECP/SelectConditioningPage"}},
        {12, {"DOOR", "AirbusFBW/ECP/SelectDoorOxyPage"}},
        {13, {"WHEEL", "AirbusFBW/ECP/SelectWheelPage"}},
        {14, {"F/CTL", "AirbusFBW/ECP/SelectFlightControlPage"}},
        {15, {"ALL", "AirbusFBW/ECAMAll"}},
        {16, {"CLR LEFT", "AirbusFBW/ECP/CaptainClear"}},
        {17, {"EMPTY 3", ""}},
        {18, {"STS", "AirbusFBW/ECP/SelectStatusPage"}},
        {19, {"RCL", "AirbusFBW/ECAMRecall"}},
        {20, {"EMPTY 4", ""}},
        {21, {"CLR RIGHT", "AirbusFBW/ECP/CopilotClear"}}};
    return buttons;
}

void TolissECAMProfile::buttonPressed(const ECAMButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    datarefManager->executeCommand(button->dataref.c_str(), phase);
}

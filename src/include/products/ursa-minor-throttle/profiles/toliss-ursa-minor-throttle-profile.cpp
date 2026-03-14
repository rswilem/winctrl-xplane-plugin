#include "toliss-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>

TolissUrsaMinorThrottleProfile::TolissUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this, product](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");

        updateDisplays();
        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/YawTrimPosition", [this, product](float trimPosition) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this, product](int annunMode) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/OHPLightsATA70_Raw");

        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/OHPLightsATA70_Raw", [this, product](const std::vector<float> &panelLights) {
        if (panelLights.size() < 13) {
            return;
        }

        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FAULT, panelLights[10] > std::numeric_limits<float>::epsilon() || isAnnunTest() ? 1 : 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, panelLights[11] > std::numeric_limits<float>::epsilon() || isAnnunTest() ? 1 : 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FAULT, panelLights[12] > std::numeric_limits<float>::epsilon() || isAnnunTest() ? 1 : 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, panelLights[13] > std::numeric_limits<float>::epsilon() || isAnnunTest() ? 1 : 0);
    });
}

TolissUrsaMinorThrottleProfile::~TolissUrsaMinorThrottleProfile() {
    Dataref::getInstance()->unbind("AirbusFBW/PanelBrightnessLevel");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("AirbusFBW/AnnunMode");
    Dataref::getInstance()->unbind("AirbusFBW/YawTrimPosition");
    Dataref::getInstance()->unbind("AirbusFBW/OHPLightsATA70_Raw");
}

bool TolissUrsaMinorThrottleProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &TolissUrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG L master ON", "AirbusFBW/ENG1MasterSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 1}},
        {1, {"ENG L master OFF", "AirbusFBW/ENG1MasterSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},
        {2, {"ENG R master ON", "AirbusFBW/ENG2MasterSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 1}},
        {3, {"ENG R master OFF", "AirbusFBW/ENG2MasterSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},
        {4, {"L Fault", ""}},
        {5, {"R Fault", ""}},
        {6, {"ENG mode CRANK", "AirbusFBW/ENGModeSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},
        {7, {"ENG mode NORMAL", "AirbusFBW/ENGModeSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 1}},
        {8, {"ENG mode START", "AirbusFBW/ENGModeSwitch", UrsaMinorThrottleDatarefType::SET_VALUE, 2}},
        {9, {"AT disconnect Left", "sim/autopilot/autothrottle_off"}},
        {10, {"AT disconnect Right", ""}}, // We could map to the same as above, but not mapping anything lets the user assign a different command if desired.
        {11, {"TOGA L", ""}},
        {12, {"MCT L", ""}},
        {13, {"CLB L", ""}},
        {14, {"IDLE L", ""}},
        {15, {"REV Idle L", ""}},
        {16, {"REV Full L", ""}},
        {17, {"TOGA R", ""}},
        {18, {"MCT R", ""}},
        {19, {"CLB R", ""}},
        {20, {"IDLE R", ""}},
        {21, {"REV Idle R", ""}},
        {22, {"REV Full R", ""}},

        {23, {"Engine mode selector pushed", ""}},

        {24, {"Rudder trim Reset", "sim/flight_controls/rudder_trim_center", UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED}},
        {25, {"Rudder trim Nose Left", "sim/flight_controls/rudder_trim_left", UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED}},
        {26, {"Rudder trim Idle", ""}},
        {27, {"Rudder trim Nose Right", "sim/flight_controls/rudder_trim_right", UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED}},

        {28, {"Park brake OFF", "AirbusFBW/ParkBrake", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},
        {29, {"Park brake ON", "AirbusFBW/ParkBrake", UrsaMinorThrottleDatarefType::SET_VALUE, 1}},

        {30, {"FLAP Full", "AirbusFBW/FlapLeverRatio", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {31, {"FLAP 3", "AirbusFBW/FlapLeverRatio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.75}},
        {32, {"FLAP 2", "AirbusFBW/FlapLeverRatio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.5}},
        {33, {"FLAP 1", "AirbusFBW/FlapLeverRatio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.25}},
        {34, {"FLAP 0", "AirbusFBW/FlapLeverRatio", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},

        {35, {"Speedbrake full", ""}},
        {36, {"Speedbrake half", ""}},
        {37, {"Speedbrake stowed", ""}},
        {38, {"Speedbrake armed", "sim/cockpit2/controls/speedbrake_ratio", UrsaMinorThrottleDatarefType::TOLISS_SPEEDBRAKE, -0.5}},

        {39, {"Reversers active L", ""}},
        {40, {"Reversers active R", ""}},
    };

    return buttons;
}

void TolissUrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == UrsaMinorThrottleDatarefType::TOLISS_SPEEDBRAKE) {
        bool shouldArm = phase == xplm_CommandBegin;
        datarefManager->set<float>(button->dataref.c_str(), shouldArm ? static_cast<float>(button->value) : 0.0f);
    } else if (button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<double>(button->dataref.c_str(), static_cast<double>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void TolissUrsaMinorThrottleProfile::updateDisplays() {
    if (!product) {
        return;
    }

    float trim = Dataref::getInstance()->get<float>("AirbusFBW/YawTrimPosition");
    float v = std::round(std::fabs(trim) * 10.0f) / 10.0f;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    std::string newTrimText = std::string(1, trim < -0.0f ? 'L' : 'R') + (v < 10.0f ? " " : "") + buf;

    if (isAnnunTest()) {
        newTrimText = "R88.8";
    }

    if (newTrimText != trimText) {
        trimText = newTrimText;
        product->setLCDText(trimText);
    }
}

bool TolissUrsaMinorThrottleProfile::isAnnunTest() {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2;
}

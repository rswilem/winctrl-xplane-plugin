#include "laminar-a333-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>

LaminarA333UrsaMinorThrottleProfile::LaminarA333UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    // Index 13 is the pedestal/panel integral lighting rheostat (INTEG LT).
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 14) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit2/autopilot/autopilot_has_power");

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, hasPower ? brightness[13] * 255 : 0);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [this, product](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");

        updateDisplays();
        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("sim/flightmodel2/controls/rudder_trim", [this](float trimRatio) {
        updateDisplays();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/a333/switches/ann_light_pos", [this](float annunMode) {
        updateDisplays();
    },
        this);

    // The aircraft already bakes DIM/BRT/TEST into these values, so thresholds must stay near-zero (DIM reads 0.0075).
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/engine1_starter_fault", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FAULT, brightness > 0.001f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/engine1_fire", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, brightness > 0.001f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/engine2_starter_fault", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FAULT, brightness > 0.001f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/engine2_fire", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, brightness > 0.001f ? 1 : 0);
    },
        this);
}

bool LaminarA333UrsaMinorThrottleProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/A333/ckpt_temp");
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &LaminarA333UrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG L master ON", "sim/starters/engage_starter_1"}},
        {1, {"ENG L master OFF", "sim/starters/shut_down_1"}},
        {2, {"ENG R master ON", "sim/starters/engage_starter_2"}},
        {3, {"ENG R master OFF", "sim/starters/shut_down_2"}},
        {4, {"L Fault", ""}},
        {5, {"R Fault", ""}},
        {6, {"ENG mode CRANK", "sim/cockpit2/engine/actuators/eng_mode_selector", UrsaMinorThrottleDatarefType::SET_VALUE, -1}},
        {7, {"ENG mode NORMAL", "sim/cockpit2/engine/actuators/eng_mode_selector", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},
        {8, {"ENG mode START", "sim/cockpit2/engine/actuators/eng_mode_selector", UrsaMinorThrottleDatarefType::SET_VALUE, 1}},
        {9, {"AT disconnect Left", "laminar/A333/autopilot/a_thr_off_eng1"}},
        {10, {"AT disconnect Right", "laminar/A333/autopilot/a_thr_off_eng2"}},
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

        {28, {"Park brake OFF", "laminar/A333/switch/parking_brake_left"}},
        {29, {"Park brake ON", "laminar/A333/switch/parking_brake_right"}},

        {30, {"FLAP Full", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {31, {"FLAP 3", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.75}},
        {32, {"FLAP 2", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.5}},
        {33, {"FLAP 1", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.25}},
        {34, {"FLAP 0", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0}},

        {35, {"Speedbrake full", ""}},
        {36, {"Speedbrake half", ""}},
        {37, {"Speedbrake stowed", ""}},
        {38, {"Speedbrake armed", "sim/cockpit2/controls/speedbrake_ratio", UrsaMinorThrottleDatarefType::TOLISS_SPEEDBRAKE, -0.5}},

        {39, {"Reversers active L", ""}},
        {40, {"Reversers active R", ""}},
    };

    return buttons;
}

void LaminarA333UrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
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

void LaminarA333UrsaMinorThrottleProfile::updateDisplays() {
    if (!product) {
        return;
    }

    // Pedestal 7-segment maps the -1..1 trim ratio to +/-25.0 degrees.
    float trim = Dataref::getInstance()->get<float>("sim/flightmodel2/controls/rudder_trim") * 25.0f;
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

bool LaminarA333UrsaMinorThrottleProfile::isAnnunTest() {
    return Dataref::getInstance()->get<float>("laminar/a333/switches/ann_light_pos") >= 1.5f;
}

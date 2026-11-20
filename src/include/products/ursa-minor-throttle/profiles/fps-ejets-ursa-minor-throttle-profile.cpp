#include "fps-ejets-ursa-minor-throttle-profile.h"

#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>

namespace {
    constexpr const char *POWER_REF = "ssg/Elec/ac_alive";
    constexpr const char *PEDESTAL_BRIGHTNESS_REF = "SSG/EJET/LIGHTS/pedestal_pnl_sw";
    constexpr const char *RUDDER_TRIM_REF = "sim/flightmodel2/controls/rudder_trim";
    constexpr const char *YAW_TRIM_KNOB_REF = "ssg/CTR/roll_yaw_sw";
    constexpr const char *ENGINE_START_KNOBS = "SSG/EJET/ENG/eng_start1_sw,SSG/EJET/ENG/eng_start2_sw";
    constexpr float MAX_RUDDER_TRIM_DEGREES = 17.6f;
}

FPSEjetsUrsaMinorThrottleProfile::FPSEjetsUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>(PEDESTAL_BRIGHTNESS_REF, [this](float) {
        updateBrightness();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>(POWER_REF, [this](float) {
        updateBrightness();
        updateDisplays();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("SSG/EJET/LIGHTS/Fire_disch1_on", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FAULT, 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, brightness > 0.01f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("SSG/EJET/LIGHTS/Fire_disch2_on", [product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FAULT, 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, brightness > 0.01f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>(RUDDER_TRIM_REF, [this](float) {
        updateDisplays();
    },
        this);
}

bool FPSEjetsUrsaMinorThrottleProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

void FPSEjetsUrsaMinorThrottleProfile::updateBrightness() {
    if (!product) {
        return;
    }

    auto dr = Dataref::getInstance();
    bool hasPower = !dr->exists(POWER_REF) || dr->getCached<float>(POWER_REF) > 0.5f;
    float ratio = std::clamp(dr->getCached<float>(PEDESTAL_BRIGHTNESS_REF), 0.0f, 1.0f);

    product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, hasPower ? static_cast<uint8_t>(ratio * 255) : 0);
    product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);
    product->forceStateSync();
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &FPSEjetsUrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG L master ON", "SSG/EJET/ENG/eng_start1_sw", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {1, {"ENG L master OFF", "SSG/EJET/ENG/eng_start1_sw", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},
        {2, {"ENG R master ON", "SSG/EJET/ENG/eng_start2_sw", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {3, {"ENG R master OFF", "SSG/EJET/ENG/eng_start2_sw", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},

        {6, {"ENG mode CRANK", ""}},
        {7, {"ENG mode NORMAL", ENGINE_START_KNOBS, UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {8, {"ENG mode START", ENGINE_START_KNOBS, UrsaMinorThrottleDatarefType::SET_VALUE, 2.0}},

        {9, {"AT disconnect L", "SSG/UFMC/AT_Discon"}},
        {10, {"AT disconnect R", "SSG/UFMC/AT_Discon"}},

        {24, {"Rudder trim Reset", ""}},
        {25, {"Rudder trim Nose Left", YAW_TRIM_KNOB_REF, UrsaMinorThrottleDatarefType::SET_VALUE, -1.0}},
        {27, {"Rudder trim Nose Right", YAW_TRIM_KNOB_REF, UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},

        {28, {"Park brake OFF", "sim/flightmodel/controls/parkbrake", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},
        {29, {"Park brake ON", "sim/flightmodel/controls/parkbrake", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},

        {30, {"FLAP FULL", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 6.0 / 6.0}},
        {31, {"FLAP 4", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 4.0 / 6.0}},
        {32, {"FLAP 2", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 2.0 / 6.0}},
        {33, {"FLAP 1", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0 / 6.0}},
        {34, {"FLAP 0", "sim/cockpit2/controls/flap_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},

        {35, {"Speedbrake full", "SSG/EJET/speed_brk_hand", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {36, {"Speedbrake half", "SSG/EJET/speed_brk_hand", UrsaMinorThrottleDatarefType::SET_VALUE, 0.5}},
        {37, {"Speedbrake stowed", "SSG/EJET/speed_brk_hand", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},
    };

    return buttons;
}

void FPSEjetsUrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dr = Dataref::getInstance();

    if (button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE) {
        if (button->dataref == YAW_TRIM_KNOB_REF) {
            dr->set<float>(YAW_TRIM_KNOB_REF, phase == xplm_CommandBegin ? static_cast<float>(button->value) : 0.0f);
            return;
        }

        if (phase != xplm_CommandBegin) {
            return;
        }

        if (button->dataref == ENGINE_START_KNOBS) {
            std::stringstream ss(button->dataref);
            std::string knob;
            while (std::getline(ss, knob, ',')) {
                if (knob.empty() || dr->get<float>(knob.c_str()) < 0.5f) {
                    continue;
                }

                dr->set<float>(knob.c_str(), static_cast<float>(button->value));
            }
            return;
        }

        dr->set<float>(button->dataref.c_str(), static_cast<float>(button->value));

    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE) {
        dr->executeCommand(button->dataref.c_str());
    }
}

void FPSEjetsUrsaMinorThrottleProfile::updateDisplays() {
    if (!product) {
        return;
    }

    float trim = Dataref::getInstance()->getCached<float>(RUDDER_TRIM_REF) * MAX_RUDDER_TRIM_DEGREES;
    float v = std::round(std::fabs(trim) * 10.0f) / 10.0f;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    std::string newTrimText = std::string(1, trim < -0.0f ? 'L' : 'R') + (v < 10.0f ? " " : "") + buf;

    if (newTrimText != trimText) {
        trimText = newTrimText;
        product->setLCDText(trimText);
    }
}

#include "zibo-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>

ZiboUrsaMinorThrottleProfile::ZiboUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [this, product](std::vector<float> panelBrightness) {
        if (panelBrightness.size() < 4) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        bool hasMainBus = Dataref::getInstance()->get<bool>("laminar/B738/electric/main_bus");
        float ratio = std::clamp(hasMainBus ? panelBrightness[3] : 0.5f, 0.0f, 1.0f);
        uint8_t backlightBrightness = hasPower ? ratio * 255 : 0;

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);

        updateDisplays();
        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this, product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");

        updateDisplays();
        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/dspl_light_test", [this](std::vector<float> displayTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/electric/main_bus", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
    });

    Dataref::getInstance()->monitorExistingDataref<float>("sim/flightmodel/controls/vstab2_rud1def", [this, product](float trimPosition) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/engine1_fire", [this, product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FAULT, 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, brightness > std::numeric_limits<float>::epsilon());
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/engine2_fire", [this, product](float brightness) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FAULT, 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, brightness > std::numeric_limits<float>::epsilon());
    });
}

ZiboUrsaMinorThrottleProfile::~ZiboUrsaMinorThrottleProfile() {
    Dataref::getInstance()->unbind("laminar/B738/electric/panel_brightness");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("laminar/B738/dspl_light_test");
    Dataref::getInstance()->unbind("laminar/B738/electric/main_bus");
    Dataref::getInstance()->unbind("sim/flightmodel/controls/vstab2_rud1def");
    Dataref::getInstance()->unbind("laminar/B738/annunciator/engine1_fire");
    Dataref::getInstance()->unbind("laminar/B738/annunciator/engine2_fire");
}

bool ZiboUrsaMinorThrottleProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/autopilot/mcp_speed_dial_kts_mach");
}

void ZiboUrsaMinorThrottleProfile::update() {
    if (Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on")) {
        float gForce = Dataref::getInstance()->get<float>("sim/flightmodel/forces/g_nrml");
        float delta = fabs(gForce - lastGForce);
        lastGForce = gForce;

        bool onGround = Dataref::getInstance()->getCached<bool>("sim/flightmodel/failures/onground_any");
        uint8_t vibration = (uint8_t) std::min(255.0f, delta * (onGround ? product->vibrationMultiplier : product->vibrationMultiplier / 2.0f));
        if (vibration < 6) {
            vibration = 0;
        }

        if (lastVibration != vibration) {
            product->setVibration(vibration);
            lastVibration = vibration;
        }
    } else if (lastVibration > 0) {
        lastVibration = 0;
        product->setVibration(lastVibration);
    }
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &ZiboUrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG L master ON", "laminar/B738/engine/mixture1_idle"}},
        {1, {"ENG L master OFF", "laminar/B738/engine/mixture1_cutoff"}},
        {2, {"ENG R master ON", "laminar/B738/engine/mixture2_idle"}},
        {3, {"ENG R master OFF", "laminar/B738/engine/mixture2_cutoff"}},
        {4, {"L Fault", ""}},
        {5, {"R Fault", ""}},
        {6, {"ENG mode CRANK", "laminar/B738/rotary/eng1_start_grd,laminar/B738/rotary/eng2_start_grd", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}}, // cont, flt, grd, off
        {7, {"ENG mode NORMAL", "laminar/B738/rotary/eng1_start_off,laminar/B738/rotary/eng2_start_off", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}},
        {8, {"ENG mode START", "laminar/B738/rotary/eng1_start_cont,laminar/B738/rotary/eng2_start_cont", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}},
        {9, {"AT disconnect Left", "laminar/B738/autopilot/left_at_dis_press"}},
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

        {24, {"Rudder trim Reset", "sim/flight_controls/rudder_trim_center"}},
        {25, {"Rudder trim Nose Left", "sim/flight_controls/rudder_trim_left"}},
        {26, {"Rudder trim Idle", ""}},
        {27, {"Rudder trim Nose Right", "sim/flight_controls/rudder_trim_right"}},

        {28, {"Park brake OFF", "laminar/B738/parking_brake_pos,laminar/B738/push_button/park_brake_on_off,laminar/B738/push_button/park_brake_on_off", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},
        {29, {"Park brake ON", "laminar/B738/parking_brake_pos,laminar/B738/push_button/park_brake_on_off,laminar/B738/push_button/park_brake_on_off", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},

        {30, {"FLAP Full", "laminar/B738/push_button/flaps_30", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 5}}, // (1, 2, 5, 10, 15, 25, 30, 40)
        {31, {"FLAP 3", "laminar/B738/push_button/flaps_25", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 4}},
        {32, {"FLAP 2", "laminar/B738/push_button/flaps_15", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 3}},
        {33, {"FLAP 1", "laminar/B738/push_button/flaps_5", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 2}},
        {34, {"FLAP 0", "laminar/B738/push_button/flaps_0", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 1}},

        {35, {"Speedbrake full", ""}},
        {36, {"Speedbrake half", ""}},
        {37, {"Speedbrake stowed", ""}},
        {38, {"Speedbrake armed", ""}}, // laminar/B738/flt_ctrls/speedbrake_lever, armed = laminar/B738/flt_ctrls/speedbrake_lever=0.0889, 0 is stowed and 1 is deployed

        {39, {"Reversers active L", ""}},
        {40, {"Reversers active R", ""}},
    };

    return buttons;
}

void ZiboUrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> parts;
        while (std::getline(ss, item, ',')) {
            parts.push_back(item);
        }

        auto posRef = parts[0];
        auto leftCmd = parts[1];
        auto rightCmd = parts[2];

        int current = datarefManager->get<int>(posRef.c_str());
        int target = static_cast<int>(button->value);

        if (current < target) {
            for (int i = current; i < target; i++) {
                datarefManager->executeCommand(rightCmd.c_str());
            }
        } else if (current > target) {
            for (int i = current; i > target; i--) {
                datarefManager->executeCommand(leftCmd.c_str());
            }
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE) {
        datarefManager->set<double>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> commands;
        while (std::getline(ss, item, ',')) {
            commands.push_back(item);
        }

        for (const auto &cmd : commands) {
            datarefManager->executeCommand(cmd.c_str());
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED) {
        if (datarefChangedCache[button->dataref] != static_cast<int>(button->value)) {
            datarefChangedCache[button->dataref] = static_cast<int>(button->value);
            datarefManager->executeCommand(button->dataref.c_str());
        }
    } else if (button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboUrsaMinorThrottleProfile::updateDisplays() {
    if (!product) {
        return;
    }

    float trim = Dataref::getInstance()->get<float>("sim/flightmodel/controls/vstab2_rud1def");
    float v = std::round(std::fabs(trim) * 10.0f) / 10.0f;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%.1f", v);
    std::string newTrimText = std::string(1, trim < -0.0f ? 'L' : 'R') + (v < 10.0f ? " " : "") + buf;

    if (newTrimText != trimText) {
        trimText = newTrimText;
        product->setLCDText(trimText);
    }
}

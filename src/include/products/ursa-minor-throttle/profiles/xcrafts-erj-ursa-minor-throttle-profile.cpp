#include "xcrafts-erj-ursa-minor-throttle-profile.h"

#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>
#include <vector>

XCraftsErjUrsaMinorThrottleProfile::XCraftsErjUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("XCrafts/panel_brt_1", [product](float brightness) {
        uint8_t target = static_cast<uint8_t>(brightness * 255);
        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, target);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, target);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/annunciators/engine_fires", [product](const std::vector<float> &fires) {
        if (fires.size() < 2) {
            return;
        }
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, fires[0] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, fires[1] > std::numeric_limits<float>::epsilon() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("sim/flightmodel/controls/vstab2_rud1def", [this](float) {
        updateDisplays();
    }, this);
}

bool XCraftsErjUrsaMinorThrottleProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/ERJ/MFD1/WX_TERR_status");
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &XCraftsErjUrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        // Engine master — SET_VALUE_USING_COMMANDS: "dataref,decCmd,incCmd", value = target position
        {0, {"ENG L master ON", "XCrafts/ERJ/engine1_starter_knob,XCrafts/Starter_Eng_1_down_CCW,XCrafts/Starter_Eng_1_up_CW", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},
        {1, {"ENG L master OFF", "XCrafts/ERJ/engine1_starter_knob,XCrafts/Starter_Eng_1_down_CCW,XCrafts/Starter_Eng_1_up_CW", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},
        {2, {"ENG R master ON", "XCrafts/ERJ/engine2_starter_knob,XCrafts/Starter_Eng_2_down_CCW,XCrafts/Starter_Eng_2_up_CW", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},
        {3, {"ENG R master OFF", "XCrafts/ERJ/engine2_starter_knob,XCrafts/Starter_Eng_2_down_CCW,XCrafts/Starter_Eng_2_up_CW", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},

        // Engine mode selector
        {6, {"ENG mode CRANK", "XCrafts/Starter_Eng_1_up_CW,XCrafts/Starter_Eng_2_up_CW", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}},
        {7, {"ENG mode NORMAL", "XCrafts/Starter_Eng_1_down_CCW,XCrafts/Starter_Eng_2_down_CCW", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}},
        {8, {"ENG mode START", "XCrafts/Starter_Eng_1_up_CW,XCrafts/Starter_Eng_2_up_CW", UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE}},

        // TOGA
        {9, {"TOGA", "XCrafts/ERJ/TOGA"}},
        {10, {"TOGA", "XCrafts/ERJ/TOGA"}},

        // Throttle detents 11-22: left unbound (user assignment)

        // Rudder trim
        {24, {"Rudder trim Reset", "sim/flight_controls/rudder_trim_center"}},
        {25, {"Rudder trim Nose Left", "sim/flight_controls/rudder_trim_left", UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED}},
        {27, {"Rudder trim Nose Right", "sim/flight_controls/rudder_trim_right", UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED}},

        // Parking brake
        {28, {"Park brake OFF", "XCrafts/ERJ/parking_brake_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},
        {29, {"Park brake ON", "XCrafts/ERJ/parking_brake_ratio", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},

        // Flaps
        {30, {"FLAP FULL", "XCrafts/ERJ/flap_lever_ratio*6,sim/flight_controls/flaps_down,sim/flight_controls/flaps_up", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 6.0}},
        {31, {"FLAP 4", "XCrafts/ERJ/flap_lever_ratio*6,sim/flight_controls/flaps_down,sim/flight_controls/flaps_up", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
        {32, {"FLAP 2", "XCrafts/ERJ/flap_lever_ratio*6,sim/flight_controls/flaps_down,sim/flight_controls/flaps_up", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {33, {"FLAP 1", "XCrafts/ERJ/flap_lever_ratio*6,sim/flight_controls/flaps_down,sim/flight_controls/flaps_up", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {34, {"FLAP 0", "XCrafts/ERJ/flap_lever_ratio*6,sim/flight_controls/flaps_down,sim/flight_controls/flaps_up", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},

        // Speedbrake
        {35, {"Speedbrake full", "XCrafts/ERJ/flight_controls/speed_brake_handle", UrsaMinorThrottleDatarefType::SET_VALUE, 1.0}},
        {36, {"Speedbrake half", "XCrafts/ERJ/flight_controls/speed_brake_handle", UrsaMinorThrottleDatarefType::SET_VALUE, 0.5}},
        {37, {"Speedbrake stowed", "XCrafts/ERJ/flight_controls/speed_brake_handle", UrsaMinorThrottleDatarefType::SET_VALUE, 0.0}},
    };

    return buttons;
}

void XCraftsErjUrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dr = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE) {
        dr->set<double>(button->dataref.c_str(), button->value);

    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS) {
        std::stringstream ss(button->dataref);
        std::string datarefToken, decCmd, incCmd;
        std::getline(ss, datarefToken, ',');
        std::getline(ss, decCmd, ',');
        std::getline(ss, incCmd, ',');

        float multiplier = 1.0f;
        size_t starPos = datarefToken.find('*');
        if (starPos != std::string::npos) {
            multiplier = std::stof(datarefToken.substr(starPos + 1));
            datarefToken = datarefToken.substr(0, starPos);
        }

        float rawValue = dr->get<float>(datarefToken.c_str());
        int current = static_cast<int>(std::round(rawValue * multiplier));
        int target = static_cast<int>(std::round(button->value));

        if (current < target) {
            for (int i = current; i < target; i++) {
                dr->executeCommand(incCmd.c_str());
            }
        } else if (current > target) {
            for (int i = current; i > target; i--) {
                dr->executeCommand(decCmd.c_str());
            }
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_MULTIPLE_CMD_ONCE) {
        std::stringstream ss(button->dataref);
        std::string cmd;
        while (std::getline(ss, cmd, ',')) {
            if (cmd.empty()) {
                continue;
            }

            if (cmd == "XCrafts/Starter_Eng_1_up_CW" || cmd == "XCrafts/Starter_Eng_1_down_CCW") {
                int knob = dr->get<int>("XCrafts/ERJ/engine1_starter_knob");
                if (cmd == "XCrafts/Starter_Eng_1_up_CW" && knob != 1) {
                    continue;
                }
                if (cmd == "XCrafts/Starter_Eng_1_down_CCW" && knob != 2) {
                    continue;
                }
            } else if (cmd == "XCrafts/Starter_Eng_2_up_CW" || cmd == "XCrafts/Starter_Eng_2_down_CCW") {
                int knob = dr->get<int>("XCrafts/ERJ/engine2_starter_knob");
                if (cmd == "XCrafts/Starter_Eng_2_up_CW" && knob != 1) {
                    continue;
                }
                if (cmd == "XCrafts/Starter_Eng_2_down_CCW" && knob != 2) {
                    continue;
                }
            }

            dr->executeCommand(cmd.c_str());
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE) {
        dr->executeCommand(button->dataref.c_str());

    } else if (button->datarefType == UrsaMinorThrottleDatarefType::EXECUTE_CMD_PHASED) {
        dr->executeCommand(button->dataref.c_str(), phase);
    }
}

void XCraftsErjUrsaMinorThrottleProfile::updateDisplays() {
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

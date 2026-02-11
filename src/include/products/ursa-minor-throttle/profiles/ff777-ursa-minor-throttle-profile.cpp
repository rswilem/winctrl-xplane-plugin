#include "ff777-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>

FF777UrsaMinorThrottleProfile::FF777UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lights/aisle", [this, product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("1-sim/output/mcp/ok");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);

        updateDisplays();
        product->forceStateSync();
    });

    // We abuse the GPU hatch dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/anim/hatchGPU", [product](bool gpuHatchOpen) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/output/mcp/ok", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/glareshield");
    });

    Dataref::getInstance()->monitorExistingDataref<float>("sim/flightmodel/controls/vstab2_rud1def", [this, product](float trimPosition) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cutoffLeftLGT", [this, product](bool isOn) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FAULT, isOn ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cutoffRightLGT", [this, product](bool isOn) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FAULT, isOn ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/engLeftFireDISCH", [this, product](bool isOn) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, isOn ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/engRightFireDISCH", [this, product](bool isOn) {
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, isOn ? 1 : 0);
    });
}

FF777UrsaMinorThrottleProfile::~FF777UrsaMinorThrottleProfile() {
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("1-sim/ckpt/lights/glareshield");
    Dataref::getInstance()->unbind("1-sim/anim/hatchGPU");
    Dataref::getInstance()->unbind("1-sim/output/mcp/ok");
    Dataref::getInstance()->unbind("sim/flightmodel/controls/vstab2_rud1def");
    Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/cutoffLeftLGT");
    Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/cutoffRightLGT");
    Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/engLeftFireDISCH");
    Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/engRightFireDISCH");
}

bool FF777UrsaMinorThrottleProfile::IsEligible() {
    // FF777 datarefs that don't exist on the FF767
    return Dataref::getInstance()->exists("1-sim/ckpt/mcpApLButton/anim") &&
           Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

void FF777UrsaMinorThrottleProfile::update() {
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

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &FF777UrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG L master ON", "1-sim/ckpt/cutoffLeftLever/anim,1-sim/command/cutoffLeftLever_trigger,1-sim/command/cutoffLeftLever_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},
        {1, {"ENG L master OFF", "1-sim/ckpt/cutoffLeftLever/anim,1-sim/command/cutoffLeftLever_trigger,1-sim/command/cutoffLeftLever_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},
        {2, {"ENG R master ON", "1-sim/ckpt/cutoffRightLever/anim,1-sim/command/cutoffRightLever_trigger,1-sim/command/cutoffRightLever_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},
        {3, {"ENG R master OFF", "1-sim/ckpt/cutoffRightLever/anim,1-sim/command/cutoffRightLever_trigger,1-sim/command/cutoffRightLever_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},
        {4, {"L Fault", ""}},
        {5, {"R Fault", ""}},
        {6, {"ENG mode CRANK", "1-sim/command/eecStartLeftSwitch_set_0"}},  // 1-sim/ckpt/eecStartLeftSwitch/anim 0,1,2 (0=start, 1=norm, 2=spark continous)
        {7, {"ENG mode NORMAL", "1-sim/command/eecStartLeftSwitch_set_1"}}, // 1-sim/command/eecStartLeftSwitch_set_0, or 1-sim/command/eecStartRightSwitch_switch+
        {8, {"ENG mode START", "1-sim/command/eecStartLeftSwitch_set_2"}},
        {9, {"AT disconnect Left", "1-sim/command/apDiscLeftButton_button"}},
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

        {24, {"Rudder trim Reset", "1-sim/command/rudderTrimCancleButton_button"}},
        {25, {"Rudder trim Nose Left", "1-sim/command/rudderTrimRotary_rotary-"}},
        {26, {"Rudder trim Idle", ""}},
        {27, {"Rudder trim Nose Right", "1-sim/command/rudderTrimRotary_rotary+"}},

        {28, {"Park brake OFF", "sim/flightmodel/controls/parkbrake,1-sim/command/parkbrake_trigger,1-sim/command/parkbrake_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 0}},
        {29, {"Park brake ON", "sim/flightmodel/controls/parkbrake,1-sim/command/parkbrake_trigger,1-sim/command/parkbrake_trigger", UrsaMinorThrottleDatarefType::SET_VALUE_USING_COMMANDS, 1}},

        {30, {"FLAP Full", "1-sim/command/flapLever_set_6", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 5}}, // 0=0, 1=1, 2=5, 3=10, 4=15, 5=25, 6=30
        {31, {"FLAP 3", "1-sim/command/flapLever_set_5", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 4}},
        {32, {"FLAP 2", "1-sim/command/flapLever_set_4", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 3}},
        {33, {"FLAP 1", "1-sim/command/flapLever_set_2", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 2}},
        {34, {"FLAP 0", "1-sim/command/flapLever_set_0", UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE_IF_CHANGED, 1}},

        {35, {"Speedbrake full", ""}},
        {36, {"Speedbrake half", ""}},
        {37, {"Speedbrake stowed", ""}},
        {38, {"Speedbrake armed", ""}}, // 1-sim/ckpt/spoilerLever/stat (0=stowed, 1=armed, 2=deployed)

        {39, {"Reversers active L", ""}},
        {40, {"Reversers active R", ""}},
    };

    return buttons;
}

void FF777UrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
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

void FF777UrsaMinorThrottleProfile::updateDisplays() {
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

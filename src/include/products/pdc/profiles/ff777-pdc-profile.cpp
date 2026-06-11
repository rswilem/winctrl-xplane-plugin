#include "ff777-pdc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>

FF777PDCProfile::FF777PDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lights/glareshield", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("1-sim/output/mcp/ok");
        uint8_t target = hasPower ? brightness * 255 : 0;
        product->setLedBrightness(PDCLed::BACKLIGHT, target);

        product->forceStateSync();
    }, this);

    // We abuse the GPU hatch dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/anim/hatchGPU", [product](bool gpuHatchOpen) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/output/mcp/ok", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/glareshield");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/ckpt/indLightTestSwitch/anim", [this, product](int isTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
    }, this);
}

FF777PDCProfile::~FF777PDCProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool FF777PDCProfile::IsEligible() {
    // FF777 datarefs that don't exist on the FF767
    return Dataref::getInstance()->exists("1-sim/ckpt/mcpApLButton/anim") &&
           Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &FF777PDCProfile::buttonDefs() const {
    const std::string pilotSide = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN ? "cpt" : "fo";
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", "1-sim/command/" + pilotSide + "HsiFpvButton_button"}},
                        {{1, 1}, {"MTRS", "1-sim/command/" + pilotSide + "HsiMtrsButton_button"}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", "1-sim/command/" + pilotSide + "HsiWxrButton_button"}},
                        {{3, 4}, {"STA", "1-sim/command/" + pilotSide + "HsiStaButton_button"}},
                        {{4, 5}, {"WPT", "1-sim/command/" + pilotSide + "HsiWptButton_button"}},
                        {{5, 6}, {"ARPT", "1-sim/command/" + pilotSide + "HsiArptButton_button"}},
                        {{6, 7}, {"DATA", "1-sim/command/" + pilotSide + "HsiDataButton_button"}},
                        {{7, 8}, {"POS", "1-sim/command/" + pilotSide + "HsiPosButton_button"}},
                        {{8, 9}, {"TERR", "1-sim/command/" + pilotSide + "HsiTerrButton_button"}},
                        {{9, 10}, {"LEFT VOR1", "1-sim/ckpt/" + pilotSide + "HsiVorLSwitch/anim,1-sim/command/" + pilotSide + "HsiVorLSwitch_button,1-sim/command/" + pilotSide + "HsiVorLSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}}, //1-sim/ckpt/"+pilotSide+"HsiVorLSwitch/anim -1 and 0 and 1 for VOR
                        {{10, 11}, {"LEFT OFF", "1-sim/ckpt/" + pilotSide + "HsiVorLSwitch/anim,1-sim/command/" + pilotSide + "HsiVorLSwitch_button,1-sim/command/" + pilotSide + "HsiVorLSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{11, 12}, {"LEFT ADF1", "1-sim/ckpt/" + pilotSide + "HsiVorLSwitch/anim,1-sim/command/" + pilotSide + "HsiVorLSwitch_button,1-sim/command/" + pilotSide + "HsiVorLSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
                        {{12, 13}, {"RIGHT VOR2", "1-sim/ckpt/" + pilotSide + "HsiVorRSwitch/anim,1-sim/command/" + pilotSide + "HsiVorRSwitch_button,1-sim/command/" + pilotSide + "HsiVorRSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{13, 14}, {"RIGHT OFF", "1-sim/ckpt/" + pilotSide + "HsiVorRSwitch/anim,1-sim/command/" + pilotSide + "HsiVorRSwitch_button,1-sim/command/" + pilotSide + "HsiVorRSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{14, 15}, {"RIGHT ADF2", "1-sim/ckpt/" + pilotSide + "HsiVorRSwitch/anim,1-sim/command/" + pilotSide + "HsiVorRSwitch_button,1-sim/command/" + pilotSide + "HsiVorRSwitch_trigger", PDCDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
                        {{15, 16}, {"Mins RST", "1-sim/command/" + pilotSide + "HsiRstButton_button"}},
                        {{16, 17}, {"VOR MAP CTR", "1-sim/command/" + pilotSide + "HsiCtrButton_button"}},
                        {{17, 18}, {"RANGE TFC", "1-sim/command/" + pilotSide + "HsiTfcButton_button"}},
                        {{18, 19}, {"Baro STD", "1-sim/command/" + pilotSide + "HsiStdButton_button"}},
                        {{-1, 20}, {"3M Range Minus", "1-sim/command/" + pilotSide + "HsiRangeSwitch_switch-"}},
                        {{-1, 21}, {"3M Range Plus", "1-sim/command/" + pilotSide + "HsiRangeSwitch_switch+"}},
                        {{21, 22}, {"Baro knob left fast", "1-sim/command/" + pilotSide + "HsiBaroRotary_rotary-", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{22, 23}, {"Baro knob right fast", "1-sim/command/" + pilotSide + "HsiBaroRotary_rotary+", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{23, 24}, {"Mins RADIO", "1-sim/command/" + pilotSide + "HsiMinsModeRotary_set_0"}},
                        {{24, 25}, {"Mins BARO", "1-sim/command/" + pilotSide + "HsiMinsModeRotary_set_1"}},
                        {{25, 26}, {"Baro inHg", "1-sim/command/" + pilotSide + "HsiBaroModeRotary_set_0"}},
                        {{26, 27}, {"Baro HPA", "1-sim/command/" + pilotSide + "HsiBaroModeRotary_set_1"}},
                        {{27, 28}, {"Map APP", "1-sim/command/" + pilotSide + "HsiModeSwitch_set_0"}},
                        {{28, 29}, {"Map VOR", "1-sim/command/" + pilotSide + "HsiModeSwitch_set_1"}},
                        {{29, 30}, {"Map MAP", "1-sim/command/" + pilotSide + "HsiModeSwitch_set_2"}},
                        {{30, 31}, {"Map PLN", "1-sim/command/" + pilotSide + "HsiModeSwitch_set_3"}},
                        {{31, -1}, {"3N Map range 5", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_0"}}, // Not available on the 777
                        {{32, -1}, {"3N Map range 10", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_0"}},
                        {{33, -1}, {"3N Map range 20", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_1"}},
                        {{34, -1}, {"3N Map range 40", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_2"}},
                        {{35, -1}, {"3N Map range 80", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_3"}},
                        {{36, -1}, {"3N Map range 160", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_4"}},
                        {{37, -1}, {"3N Map range 320", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_5"}},
                        {{38, -1}, {"3N Map range 640", "1-sim/command/" + pilotSide + "HsiRangeSwitch_set_6"}},
                        {{19, 32}, {"Mins knob left fast", "1-sim/command/" + pilotSide + "HsiMinsRotary_rotary-", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{39, 33}, {"Mins knob left slow", "1-sim/command/" + pilotSide + "HsiMinsRotary_rotary-", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", "1-sim/command/" + pilotSide + "HsiMinsRotary_rotary+", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{20, 36}, {"Mins knob right fast", "1-sim/command/" + pilotSide + "HsiMinsRotary_rotary+", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{42, 37}, {"Baro knob left slow", "1-sim/command/" + pilotSide + "HsiBaroRotary_rotary-", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", "1-sim/command/" + pilotSide + "HsiBaroRotary_rotary+", PDCDatarefType::EXECUTE_CMD_PHASED}},
                    })
        .first->second;
}

void FF777PDCProfile::update() {
    // No periodic updates
}

void FF777PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE_USING_COMMANDS) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> parts;
        while (std::getline(ss, item, ',')) {
            parts.push_back(item);
        }

        auto posRef = parts[0];
        auto fullUpCmd = parts[1];
        auto moveOneDownCmd = parts[2];

        datarefManager->executeCommand(fullUpCmd.c_str());
        // Due to the command above, current value is now 1. We need to move down to the target value.

        int steps = 1 - static_cast<int>(button->value);
        for (int i = 0; i < std::abs(steps); i++) {
            datarefManager->executeCommand(moveOneDownCmd.c_str());
        }

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

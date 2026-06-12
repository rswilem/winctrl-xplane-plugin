#include "zibo-pdc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

ZiboPDCProfile::ZiboPDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [this, product](const std::vector<float> &panelBrightness) {
        if (panelBrightness.size() < 1) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        bool hasMainBus = Dataref::getInstance()->get<bool>("laminar/B738/electric/main_bus");
        float ratio = std::clamp(hasMainBus ? panelBrightness[0] : 0.5f, 0.0f, 1.0f);
        uint8_t brightness = hasPower ? ratio * 255 : 0;
        product->setLedBrightness(PDCLed::BACKLIGHT, brightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/dspl_light_test", [this](const std::vector<float> &displayTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/electric/main_bus", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
    }, this);
}

bool ZiboPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &ZiboPDCProfile::buttonDefs() const {
    const std::string pilotSide = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN ? "capt" : "fo";
    const std::string pilotOrCopilot = pilotSide == "capt" ? "pilot" : "copilot";
    const std::string cptOrFo = pilotSide == "capt" ? "cpt" : "fo";
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/fpv_press"}},
                        {{1, 1}, {"MTRS", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/mtrs_press"}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/wxr_press"}},
                        {{3, 4}, {"STA", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/sta_press"}},
                        {{4, 5}, {"WPT", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/wpt_press"}},
                        {{5, 6}, {"ARPT", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/arpt_press"}},
                        {{6, 7}, {"DATA", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/data_press"}},
                        {{7, 8}, {"POS", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/pos_press"}},
                        {{8, 9}, {"TERR", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/terr_press"}},
                        {{9, 10}, {"LEFT VOR1", "laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{10, 11}, {"LEFT OFF", "laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{11, 12}, {"LEFT ADF1", "laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor1_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
                        {{12, 13}, {"RIGHT VOR2", "laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{13, 14}, {"RIGHT OFF", "laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{14, 15}, {"RIGHT ADF2", "laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_pos,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_dn,laminar/B738/EFIS_control/" + pilotSide + "/vor2_off_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
                        {{15, 16}, {"Mins RST", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/rst_press"}},
                        {{16, 17}, {"VOR MAP CTR", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/ctr_press"}},
                        {{17, 18}, {"RANGE TFC", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/tfc_press"}},
                        {{18, 19}, {"Baro STD", "laminar/B738/EFIS_control/" + pilotSide + "/push_button/std_press"}},
                        {{-1, 20}, {"3M Range Minus", "laminar/B738/EFIS_control/" + pilotSide + "/map_range_dn"}}, // PDC3N - laminar/B738/EFIS/"+pilotSide+"/map_range,0,1,2,3,4,5,6,7
                        {{-1, 21}, {"3M Range Plus", "laminar/B738/EFIS_control/" + pilotSide + "/map_range_up"}},
                        {{21, 22}, {"Baro knob left fast", "laminar/B738/" + pilotOrCopilot + "/barometer_down", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{22, 23}, {"Baro knob right fast", "laminar/B738/" + pilotOrCopilot + "/barometer_up", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{23, 24}, {"Mins RADIO", "laminar/B738/EFIS_control/" + cptOrFo + "/minimums,laminar/B738/EFIS_control/" + cptOrFo + "/minimums_up,laminar/B738/EFIS_control/" + cptOrFo + "/minimums_dn", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}}, // Caution, up and down are inverted
                        {{24, 25}, {"Mins BARO", "laminar/B738/EFIS_control/" + cptOrFo + "/minimums,laminar/B738/EFIS_control/" + cptOrFo + "/minimums_up,laminar/B738/EFIS_control/" + cptOrFo + "/minimums_dn", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},  // Caution, up and down are inverted
                        {{25, 26}, {"Baro inHg", "laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa,laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa_dn,laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                        {{26, 27}, {"Baro HPA", "laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa,laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa_dn,laminar/B738/EFIS_control/" + pilotSide + "/baro_in_hpa_up", PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                        {{27, 28}, {"Map APP", "laminar/B738/EFIS_control/" + pilotSide + "/map_mode_pos", PDCDatarefType::SET_VALUE, 0.0}},
                        {{28, 29}, {"Map VOR", "laminar/B738/EFIS_control/" + pilotSide + "/map_mode_pos", PDCDatarefType::SET_VALUE, 1.0}},
                        {{29, 30}, {"Map MAP", "laminar/B738/EFIS_control/" + pilotSide + "/map_mode_pos", PDCDatarefType::SET_VALUE, 2.0}},
                        {{30, 31}, {"Map PLN", "laminar/B738/EFIS_control/" + pilotSide + "/map_mode_pos", PDCDatarefType::SET_VALUE, 3.0}},
                        {{31, -1}, {"3N Map range 5", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 0.0}},
                        {{32, -1}, {"3N Map range 10", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 1.0}},
                        {{33, -1}, {"3N Map range 20", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 2.0}},
                        {{34, -1}, {"3N Map range 40", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 3.0}},
                        {{35, -1}, {"3N Map range 80", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 4.0}},
                        {{36, -1}, {"3N Map range 160", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 5.0}},
                        {{37, -1}, {"3N Map range 320", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 6.0}},
                        {{38, -1}, {"3N Map range 640", "laminar/B738/EFIS/" + pilotSide + "/map_range", PDCDatarefType::SET_VALUE, 7.0}},
                        {{19, 32}, {"Mins knob left fast", std::string("laminar/B738/pfd/dh_") + pilotOrCopilot + "_dn", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{39, 33}, {"Mins knob left slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", std::string("laminar/B738/pfd/dh_") + pilotOrCopilot + "_up", PDCDatarefType::EXECUTE_CMD_PHASED}},
                        {{42, 37}, {"Baro knob left slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void ZiboPDCProfile::update() {
    if (minimumsDelta != 0 && XPLMGetElapsedTime() - minimumsLastCommandTime >= 0.1f) {
        minimumsLastCommandTime = XPLMGetElapsedTime();
        changeMinimums();
    }

    if (baroDelta != 0 && XPLMGetElapsedTime() - baroLastCommandTime >= 0.1f) {
        baroLastCommandTime = XPLMGetElapsedTime();
        changeBaro();
    }
}

void ZiboPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        baroDelta = phase == xplm_CommandBegin ? static_cast<char>(button->value) : 0;
        baroLastCommandTime = XPLMGetElapsedTime() + 1.0f;
        changeBaro();
    } else if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        minimumsDelta = phase == xplm_CommandBegin ? static_cast<char>(button->value) : 0;
        minimumsLastCommandTime = XPLMGetElapsedTime() + 1.0f;
        changeMinimums();
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE_USING_COMMANDS) {
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

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        datarefManager->set<double>(button->dataref.c_str(), button->value);

    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboPDCProfile::changeMinimums() {
    if (minimumsDelta == 0) {
        return;
    }

    std::string dataref = std::string("laminar/B738/pfd/dh_") + (product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN ? "pilot" : "copilot");
    auto datarefManager = Dataref::getInstance();
    float currentMins = datarefManager->get<float>(dataref.c_str());
    currentMins += minimumsDelta;
    datarefManager->set<float>(dataref.c_str(), currentMins);
}

void ZiboPDCProfile::changeBaro() {
    if (baroDelta == 0) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    bool isHPA = datarefManager->get<bool>((std::string("laminar/B738/EFIS_control/") + (product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN ? "capt" : "fo") + "/baro_in_hpa").c_str());
    std::string dataref = std::string("laminar/B738/EFIS/baro_sel_in_hg_") + (product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN ? "pilot" : "copilot");
    float currentBaroInHg = datarefManager->get<float>(dataref.c_str());
    if (isHPA) {
        currentBaroInHg += baroDelta * 0.02953f;
    } else {
        currentBaroInHg += baroDelta * 0.01f;
    }
    datarefManager->set<float>(dataref.c_str(), currentBaroInHg);
}
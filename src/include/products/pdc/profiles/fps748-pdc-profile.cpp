#include "fps748-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

FPS748PDCProfile::FPS748PDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    bool isSSG = IsSSGVersion();
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    Dataref::getInstance()->monitorExistingDataref<float>((altPrefix + "/LGT/glaresheld_sw").c_str(), [product, altPrefix](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
        uint8_t backlight = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;
        product->setLedBrightness(PDCLed::BACKLIGHT, backlight);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>((altPrefix + "/Elec/bus_1_powered").c_str(), [altPrefix](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/LGT/glaresheld_sw").c_str());
    }, this);
}

bool FPS748PDCProfile::IsSSGVersion() {
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

bool FPS748PDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("FPS/748/simtime") || Dataref::getInstance()->exists("SSG/748/simtime");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &FPS748PDCProfile::buttonDefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    bool isCaptain = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    std::string side = isCaptain ? "pilot" : "copilot";

    int cacheKey = (static_cast<int>(product->deviceVariant) << 1) | (isSSG ? 1 : 0);
    static std::unordered_map<int, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    return cache.try_emplace(cacheKey,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", prefix + "/B748/PFD/fpv_sw_" + side, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{1, 1}, {"MTRS", "FPS/PFD/meters_sw_" + side, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", prefix + "/B748/ND/show_wheather_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{3, 4}, {"STA", prefix + "/B748/ND/show_VOR_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{4, 5}, {"WPT", prefix + "/B748/ND/show_waypoint_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{5, 6}, {"ARPT", prefix + "/B748/ND/show_airport_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{6, 7}, {"DATA", prefix + "/B748/ND/show_NDB_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{7, 8}, {"POS", prefix + "/B748/ND/show_POS_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{8, 9}, {"TERR", prefix + "/B748/ND/show_Terr_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{9, 10}, {"LEFT VOR1", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf1") : (prefix + "/B748/MCP/ap_CP_vor_adf1"), PDCDatarefType::SET_VALUE, 1.0}},
                        {{10, 11}, {"LEFT OFF", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf1") : (prefix + "/B748/MCP/ap_CP_vor_adf1"), PDCDatarefType::SET_VALUE, 0.0}},
                        {{11, 12}, {"LEFT ADF1", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf1") : (prefix + "/B748/MCP/ap_CP_vor_adf1"), PDCDatarefType::SET_VALUE, -1.0}},
                        {{12, 13}, {"RIGHT VOR2", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf2") : (prefix + "/B748/MCP/ap_CP_vor_adf2"), PDCDatarefType::SET_VALUE, 1.0}},
                        {{13, 14}, {"RIGHT OFF", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf2") : (prefix + "/B748/MCP/ap_CP_vor_adf2"), PDCDatarefType::SET_VALUE, 0.0}},
                        {{14, 15}, {"RIGHT ADF2", isCaptain ? (prefix + "/B748/MCP/ap_vor_adf2") : (prefix + "/B748/MCP/ap_CP_vor_adf2"), PDCDatarefType::SET_VALUE, -1.0}},
                        {{15, 16}, {"Mins RST", ""}},
                        {{16, 17}, {"VOR MAP CTR", prefix + "/B748/ND/CRT_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{17, 18}, {"RANGE TFC", prefix + "/B748/ND/show_TCAS_" + side + "_sw", PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{18, 19}, {"Baro STD", isCaptain ? "FPS/PFD/baro_standard" : "FPS/PFD/baro_standard2", PDCDatarefType::SET_VALUE, 1.0}},
                        {{-1, 20}, {"3M Range Minus", ""}},
                        {{-1, 21}, {"3M Range Plus", ""}},
                        {{21, 22}, {"Baro knob left fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{22, 23}, {"Baro knob right fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                        {{23, 24}, {"Mins RADIO", "FPS/PFD/dh_mode_sw", PDCDatarefType::SET_VALUE, 0.0}},
                        {{24, 25}, {"Mins BARO", "FPS/PFD/dh_mode_sw", PDCDatarefType::SET_VALUE, 1.0}},
                        {{25, 26}, {"Baro inHg", "FPS/PFD/baro_type_sw", PDCDatarefType::SET_VALUE, 0.0}},
                        {{26, 27}, {"Baro HPA", "FPS/PFD/baro_type_sw", PDCDatarefType::SET_VALUE, 1.0}},
                        {{27, 28}, {"Map APP", prefix + "/B748/ND/mode_" + side, PDCDatarefType::SET_VALUE, 0.0}},
                        {{28, 29}, {"Map VOR", prefix + "/B748/ND/mode_" + side, PDCDatarefType::SET_VALUE, 1.0}},
                        {{29, 30}, {"Map MAP", prefix + "/B748/ND/mode_" + side, PDCDatarefType::SET_VALUE, 2.0}},
                        {{30, 31}, {"Map PLN", prefix + "/B748/ND/mode_" + side, PDCDatarefType::SET_VALUE, 3.0}},
                        {{31, -1}, {"3N Map range 5", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 0.0}},
                        {{32, -1}, {"3N Map range 10", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 1.0}},
                        {{33, -1}, {"3N Map range 20", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 2.0}},
                        {{34, -1}, {"3N Map range 40", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 3.0}},
                        {{35, -1}, {"3N Map range 80", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 4.0}},
                        {{36, -1}, {"3N Map range 160", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 5.0}},
                        {{37, -1}, {"3N Map range 320", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 6.0}},
                        {{38, -1}, {"3N Map range 640", prefix + "/B748/ND/range_" + side, PDCDatarefType::SET_VALUE, 7.0}},
                        {{19, 32}, {"Mins knob left fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{39, 33}, {"Mins knob left slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{42, 37}, {"Baro knob left slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void FPS748PDCProfile::update() {
    if (minimumsDelta != 0 && XPLMGetElapsedTime() - minimumsLastCommandTime >= 0.1f) {
        minimumsLastCommandTime = XPLMGetElapsedTime();
        changeMinimums();
    }

    if (baroDelta != 0 && XPLMGetElapsedTime() - baroLastCommandTime >= 0.1f) {
        baroLastCommandTime = XPLMGetElapsedTime();
        changeBaro();
    }
}

void FPS748PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        baroDelta = phase == xplm_CommandBegin ? static_cast<char>(button->value) : 0;
        baroLastCommandTime = XPLMGetElapsedTime() + 1.0f;
        changeBaro();
    } else if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        minimumsDelta = phase == xplm_CommandBegin ? static_cast<char>(button->value) : 0;
        minimumsLastCommandTime = XPLMGetElapsedTime() + 1.0f;
        changeMinimums();
    } else if (button->datarefType == PDCDatarefType::SET_VALUE_MOMENTARY) {
        if (phase == xplm_CommandBegin) {
            dm->set<double>(button->dataref.c_str(), button->value);
        } else if (phase == xplm_CommandEnd) {
            dm->set<double>(button->dataref.c_str(), 0.0);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        dm->set<double>(button->dataref.c_str(), button->value);
        if (button->dataref == "FPS/PFD/dh_mode_sw") {
            dm->set<int>("FPS/AP/ra_baro_sel", static_cast<int>(button->value));
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_PHASED) {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

void FPS748PDCProfile::changeMinimums() {
    if (minimumsDelta == 0) {
        return;
    }

    auto dm = Dataref::getInstance();
    bool isBaroMode = dm->get<int>("FPS/PFD/dh_mode_sw") == 1;
    const char *dataref = isBaroMode ? "FPS/AP/minimum_baro_dailed" : "FPS/AP/minimum_radio_dailed";
    int current = dm->get<int>(dataref);
    dm->set<int>(dataref, current + minimumsDelta * 10);
}

void FPS748PDCProfile::changeBaro() {
    if (baroDelta == 0) {
        return;
    }

    bool isCaptain = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    const char *dataref = isCaptain ? "FPS/PFD/baro_act" : "FPS/PFD/baro_act2";
    auto dm = Dataref::getInstance();
    int current = dm->get<int>(dataref);
    dm->set<int>(dataref, current + baroDelta);
}

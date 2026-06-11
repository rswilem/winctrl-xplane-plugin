#include "xcrafts-ejets-pdc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

XCraftsEjetsPDCProfile::XCraftsEjetsPDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("XCrafts/panel_brt_1", [this, product](float brightness) {
        uint8_t target = static_cast<uint8_t>(brightness * 255);
        product->setLedBrightness(PDCLed::BACKLIGHT, target);
        product->forceStateSync();
    }, this);
}

XCraftsEjetsPDCProfile::~XCraftsEjetsPDCProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool XCraftsEjetsPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/FMS/CDU_1_01");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &XCraftsEjetsPDCProfile::buttonDefs() const {
    static std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> buttons = {
        {{0, 0}, {"FPV", ""}},
        {{1, 1}, {"LS", "XCrafts/ERJ/MFD/MAP_tab", PDCDatarefType::SET_VALUE, 0.0}},
        {{-1, 2}, {"3M VSD", ""}},
        {{2, 3}, {"VOR", "XCrafts/ERJ/MFD/MAP_tab", PDCDatarefType::SET_VALUE, 1.0}},
        {{3, 4}, {"NAV", "XCrafts/ERJ/MFD/SYS_tab", PDCDatarefType::SET_VALUE, 0.0}},
        {{4, 5}, {"ARC", "XCrafts/ERJ/MFD/MAP_tab", PDCDatarefType::SET_VALUE, 2.0}},
        {{5, 6}, {"PLAN", "XCrafts/ERJ/MFD/PLAN_tab", PDCDatarefType::SET_VALUE, 0.0}},
        {{6, 7}, {"DATA", ""}},
        {{7, 8}, {"POS", ""}},
        {{8, 9}, {"TERR", ""}},
        {{9, 10}, {"WXTR", ""}},
        {{10, 11}, {"WPT", ""}},
        {{11, 12}, {"ARPT", ""}},
        {{12, 13}, {"STA", ""}},
        {{13, 14}, {"LEFT VOR1", ""}},
        {{14, 15}, {"LEFT OFF", ""}},
        {{15, 16}, {"LEFT ADF1", ""}},
        {{16, 17}, {"RIGHT VOR2", ""}},
        {{17, 18}, {"RIGHT OFF", ""}},
        {{18, 19}, {"Baro STD", "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot", PDCDatarefType::SET_VALUE, 1.0}},
        {{19, 32}, {"Mins knob left fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
        {{-1, 20}, {"Range -", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::ADD_RANGE_REPEATING, -1.0}},
        {{-1, 21}, {"Range +", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::ADD_RANGE_REPEATING, 1.0}},
        {{21, 22}, {"Baro knob left fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
        {{22, 23}, {"Baro knob right fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
        {{23, 24}, {"Mins BARO", "XCrafts/ERJ/RA_BARO_Mins", PDCDatarefType::SET_VALUE, 0.0}},
        {{24, 25}, {"Mins RA", "XCrafts/ERJ/RA_BARO_Mins", PDCDatarefType::SET_VALUE, 1.0}},
        {{25, 26}, {"Baro inHg", "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot", PDCDatarefType::SET_VALUE, 0.0}},
        {{26, 27}, {"Baro STD", "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot", PDCDatarefType::SET_VALUE, 1.0}},
        {{27, 28}, {"Map APP", ""}},
        {{28, 29}, {"Map VOR", ""}},
        {{29, 30}, {"Map MAP", "XCrafts/ERJ/MFD/MAP_tab", PDCDatarefType::SET_VALUE, 2.0}},
        {{30, 31}, {"Map PLN", "XCrafts/ERJ/MFD/PLAN_tab", PDCDatarefType::SET_VALUE, 0.0}},
        {{31, -1}, {"3N Range 10", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 0.0}},
        {{32, -1}, {"3N Range 20", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 1.0}},
        {{33, -1}, {"3N Range 40", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 2.0}},
        {{34, -1}, {"3N Range 80", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 3.0}},
        {{35, -1}, {"3N Range 160", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 4.0}},
        {{36, -1}, {"3N Range 320", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 5.0}},
        {{37, -1}, {"3N Range 640", "sim/cockpit/switches/EFIS_map_range_selector", PDCDatarefType::SET_VALUE, 6.0}},
        {{38, -1}, {"3N ADF RST", ""}},
        {{39, 33}, {"Mins knob left slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
        {{40, 34}, {"Mins knob center", ""}},
        {{41, 35}, {"Mins knob right slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
        {{20, 36}, {"Mins knob right fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
        {{42, 37}, {"Baro knob left slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
        {{43, 38}, {"Baro knob center", ""}},
        {{44, 39}, {"Baro knob right slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
    };

    return buttons;
}

void XCraftsEjetsPDCProfile::update() {
    if (minimumsDelta != 0 && XPLMGetElapsedTime() - minimumsLastCommandTime >= 0.1f) {
        minimumsLastCommandTime = XPLMGetElapsedTime();
        changeMinimums();
    }

    if (baroDelta != 0 && XPLMGetElapsedTime() - baroLastCommandTime >= 0.1f) {
        baroLastCommandTime = XPLMGetElapsedTime();
        changeBaro();
    }

    if (rangeDelta != 0 && XPLMGetElapsedTime() - rangeLastCommandTime >= 0.1f) {
        rangeLastCommandTime = XPLMGetElapsedTime();
        changeRange();
    }
}

void XCraftsEjetsPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
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
    } else if (button->datarefType == PDCDatarefType::ADD_RANGE_REPEATING) {
        rangeDelta = phase == xplm_CommandBegin ? static_cast<char>(button->value) : 0;
        rangeLastCommandTime = XPLMGetElapsedTime() + 1.0f;
        changeRange();
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        datarefManager->set<double>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void XCraftsEjetsPDCProfile::changeMinimums() {
    if (minimumsDelta == 0) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    bool isRA = datarefManager->get<int>("XCrafts/ERJ/RA_BARO_Mins") == 1;

    std::string dataref = isRA ? "sim/cockpit/misc/radio_altimeter_minimum" : "XCrafts/ERJ/baro_altimeter_minimums";
    float currentMins = datarefManager->get<float>(dataref.c_str());
    currentMins += minimumsDelta;
    datarefManager->set<float>(dataref.c_str(), currentMins);
}

void XCraftsEjetsPDCProfile::changeBaro() {
    if (baroDelta == 0) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    bool isSTD = datarefManager->get<int>("sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot") == 1;

    if (!isSTD) {
        std::string dataref = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";
        float currentBaro = datarefManager->get<float>(dataref.c_str());
        currentBaro += baroDelta * 0.01f;
        datarefManager->set<float>(dataref.c_str(), currentBaro);
    }
}

void XCraftsEjetsPDCProfile::changeRange() {
    if (rangeDelta == 0) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    float currentRange = datarefManager->get<float>("sim/cockpit/switches/EFIS_map_range_selector");
    float newRange = std::clamp(currentRange + rangeDelta, 0.0f, 6.0f);
    datarefManager->set<float>("sim/cockpit/switches/EFIS_map_range_selector", newRange);
}

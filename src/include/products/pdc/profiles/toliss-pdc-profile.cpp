#include "toliss-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

namespace {
    constexpr const char *kCustomMapMode = "custom_map_mode";
    constexpr const char *kCustomCenter = "custom_center";
    constexpr const char *kCustomStd = "custom_std";
    constexpr const char *kCustomRangeStep = "custom_range_step";

    constexpr int kNDModeNav = 2;
    constexpr int kNDModeArc = 3;
    constexpr int kNDRangeMax = 5;

    float stepInDirection(float value, bool up, float step) {
        return up ? std::floor(value / step) * step + step : std::ceil(value / step) * step - step;
    }
}

TolissPDCProfile::TolissPDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/SupplLightLevelRehostats", [product](const std::vector<float> &brightness) {
        if (brightness.empty()) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
        product->setLedBrightness(PDCLed::BACKLIGHT, hasPower ? std::clamp(brightness[0], 0.0f, 1.0f) * 255 : 0);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/SupplLightLevelRehostats");
    },
        this);
}

bool TolissPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/FCUAvail") &&
           Dataref::getInstance()->exists("AirbusFBW/NDrangeCapt");
}

std::string TolissPDCProfile::sideDataref(const char *captain, const char *firstOfficer) const {
    return product->isCaptainSide() ? captain : firstOfficer;
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &TolissPDCProfile::buttonDefs() const {
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    const std::string ndMode = sideDataref("AirbusFBW/NDmodeCapt", "AirbusFBW/NDmodeFO");
    const std::string ndRange = sideDataref("AirbusFBW/NDrangeCapt", "AirbusFBW/NDrangeFO");
    const std::string baroUnit = sideDataref("AirbusFBW/BaroUnitCapt", "AirbusFBW/BaroUnitFO");
    const std::string efis1 = sideDataref("sim/cockpit2/EFIS/EFIS_1_selection_pilot", "sim/cockpit2/EFIS/EFIS_1_selection_copilot");
    const std::string efis2 = sideDataref("sim/cockpit2/EFIS/EFIS_2_selection_pilot", "sim/cockpit2/EFIS/EFIS_2_selection_copilot");
    const std::string disp = std::string("toliss_airbus/dispcommands/") + (product->isCaptainSide() ? "Capt" : "Co");

    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", "toliss_airbus/hdgtrk_button_push"}},
                        {{1, 1}, {"MTRS", "toliss_airbus/metric_alt_button_push"}},
                        {{3, 4}, {"STA", disp + "VorDPushButton"}},
                        {{4, 5}, {"WPT", disp + "WptPushButton"}},
                        {{5, 6}, {"ARPT", disp + "ArptPushButton"}},
                        {{6, 7}, {"DATA", disp + "CstrPushButton"}},
                        {{7, 8}, {"POS", disp + "NdbPushButton"}},
                        {{8, 9}, {"TERR", sideDataref("toliss_airbus/dispcommands/TerrOnND1Toggle", "toliss_airbus/dispcommands/TerrOnND2Toggle")}},
                        {{9, 10}, {"LEFT VOR1", efis1, PDCDatarefType::SET_VALUE, 2.0}},
                        {{10, 11}, {"LEFT OFF", efis1, PDCDatarefType::SET_VALUE, 1.0}},
                        {{11, 12}, {"LEFT ADF1", efis1, PDCDatarefType::SET_VALUE, 0.0}},
                        {{12, 13}, {"RIGHT VOR2", efis2, PDCDatarefType::SET_VALUE, 2.0}},
                        {{13, 14}, {"RIGHT OFF", efis2, PDCDatarefType::SET_VALUE, 1.0}},
                        {{14, 15}, {"RIGHT ADF2", efis2, PDCDatarefType::SET_VALUE, 0.0}},
                        {{16, 17}, {"VOR MAP CTR", kCustomCenter}},
                        {{18, 19}, {"Baro STD", kCustomStd}},
                        {{-1, 20}, {"3M Range Minus", kCustomRangeStep, PDCDatarefType::EXECUTE_CMD_ONCE, -1.0}},
                        {{-1, 21}, {"3M Range Plus", kCustomRangeStep, PDCDatarefType::EXECUTE_CMD_ONCE, 1.0}},
                        {{21, 22}, {"Baro knob left fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
                        {{25, 26}, {"Baro inHg", baroUnit, PDCDatarefType::SET_VALUE, 0.0}},
                        {{26, 27}, {"Baro HPA", baroUnit, PDCDatarefType::SET_VALUE, 1.0}},
                        {{27, 28}, {"Map APP", ndMode, PDCDatarefType::SET_VALUE, 0.0}},
                        {{28, 29}, {"Map VOR", ndMode, PDCDatarefType::SET_VALUE, 1.0}},
                        {{29, 30}, {"Map MAP", kCustomMapMode}},
                        {{30, 31}, {"Map PLN", ndMode, PDCDatarefType::SET_VALUE, 4.0}},
                        {{31, -1}, {"3N Map range 5", ndRange, PDCDatarefType::SET_VALUE, 0.0}},
                        {{32, -1}, {"3N Map range 10", ndRange, PDCDatarefType::SET_VALUE, 0.0}},
                        {{33, -1}, {"3N Map range 20", ndRange, PDCDatarefType::SET_VALUE, 1.0}},
                        {{34, -1}, {"3N Map range 40", ndRange, PDCDatarefType::SET_VALUE, 2.0}},
                        {{35, -1}, {"3N Map range 80", ndRange, PDCDatarefType::SET_VALUE, 3.0}},
                        {{36, -1}, {"3N Map range 160", ndRange, PDCDatarefType::SET_VALUE, 4.0}},
                        {{37, -1}, {"3N Map range 320", ndRange, PDCDatarefType::SET_VALUE, 5.0}},
                        {{38, -1}, {"3N Map range 640", ndRange, PDCDatarefType::SET_VALUE, 5.0}},
                        {{42, 37}, {"Baro knob left slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{44, 39}, {"Baro knob right slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void TolissPDCProfile::update() {
    char baroDelta = baroFastDelta != 0 ? baroFastDelta : baroSlowDelta;
    float now = XPLMGetElapsedTime();
    if (baroDelta != 0 && now - baroLastCommandTime >= 0.1f) {
        baroLastCommandTime = now;
        changeBaro(baroDelta);
    }
}

void TolissPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        char value = static_cast<char>(button->value);
        bool isFast = value == 10 || value == -10;
        float now = XPLMGetElapsedTime();

        if (phase == xplm_CommandEnd) {
            if (isFast) {
                baroFastDelta = 0;
                baroFastReleaseTime = now;
                baroLastCommandTime = now + 1.0f;
            } else {
                baroSlowDelta = 0;
            }
        } else if (isFast) {
            baroFastDelta = value;
            baroLastCommandTime = now + 1.0f;
            changeBaro(value);
        } else {
            baroSlowDelta = value;
            baroLastCommandTime = now + 1.0f;
            if (baroFastDelta == 0 && now - baroFastReleaseTime > 0.3f) {
                changeBaro(value);
            }
        }
        return;
    }

    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref.rfind("custom_", 0) == 0) {
        handleCustomButton(button);
    } else if (button->datarefType == PDCDatarefType::SET_VALUE) {
        Dataref::getInstance()->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void TolissPDCProfile::handleCustomButton(const PDCButtonDef *button) {
    auto datarefManager = Dataref::getInstance();
    const std::string ndMode = sideDataref("AirbusFBW/NDmodeCapt", "AirbusFBW/NDmodeFO");

    if (button->dataref == kCustomMapMode) {
        datarefManager->set<float>(ndMode.c_str(), static_cast<float>(mapCentered ? kNDModeNav : kNDModeArc));
    } else if (button->dataref == kCustomCenter) {
        int mode = datarefManager->get<int>(ndMode.c_str());
        if (mode == kNDModeArc || mode == kNDModeNav) {
            mapCentered = mode == kNDModeArc;
            datarefManager->set<float>(ndMode.c_str(), static_cast<float>(mapCentered ? kNDModeNav : kNDModeArc));
        }
    } else if (button->dataref == kCustomStd) {
        bool isStd = datarefManager->get<bool>(sideDataref("AirbusFBW/BaroStdCapt", "AirbusFBW/BaroStdFO").c_str());
        const char *command = product->isCaptainSide() ? (isStd ? "toliss_airbus/capt_baro_push" : "toliss_airbus/capt_baro_pull") : (isStd ? "toliss_airbus/copilot_baro_push" : "toliss_airbus/copilot_baro_pull");
        datarefManager->executeCommand(command);
    } else if (button->dataref == kCustomRangeStep) {
        const std::string ndRange = sideDataref("AirbusFBW/NDrangeCapt", "AirbusFBW/NDrangeFO");
        int range = std::clamp(datarefManager->get<int>(ndRange.c_str()) + static_cast<int>(button->value), 0, kNDRangeMax);
        datarefManager->set<float>(ndRange.c_str(), static_cast<float>(range));
    }
}

void TolissPDCProfile::changeBaro(char delta) {
    if (delta == 0) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (datarefManager->get<bool>(sideDataref("AirbusFBW/BaroStdCapt", "AirbusFBW/BaroStdFO").c_str())) {
        return;
    }

    bool isHpa = datarefManager->get<bool>(sideDataref("AirbusFBW/BaroUnitCapt", "AirbusFBW/BaroUnitFO").c_str());
    const std::string baroDataref = sideDataref("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot", "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");
    float baroInHg = datarefManager->get<float>(baroDataref.c_str());

    bool up = delta > 0;
    float step = (delta == 10 || delta == -10) ? 10.0f : 1.0f;

    if (isHpa) {
        float hpa = stepInDirection(std::round(baroInHg * 33.8639f), up, step);
        baroInHg = hpa / 33.8639f;
    } else {
        float centiInHg = stepInDirection(std::round(baroInHg * 100.0f), up, step);
        baroInHg = centiInHg / 100.0f;
    }

    datarefManager->set<float>(baroDataref.c_str(), baroInHg);
}

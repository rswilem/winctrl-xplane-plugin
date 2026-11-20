#include "fps-ejets-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <XPLMProcessing.h>

namespace {
    constexpr const char *kRangeDetent = "custom_range_detent";
    constexpr const char *kBaroPilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";
    constexpr const char *kBaroCopilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
    constexpr const char *kBaroType = "SSG/PFD/baro_type_sw";
    constexpr const char *kRaBaroSel = "SSG/AP/ra_baro_sel";
    constexpr const char *kMinimumsBaro = "SSG/AP/minimum_baro_sw";
    constexpr const char *kMinimumsRadio = "SSG/AP/minimum_radio_sw";
    constexpr const char *kBacklight = "sim/cockpit2/electrical/instrument_brightness_ratio";

    constexpr size_t kBacklightIndex = 11;
    constexpr float kMinimumsBaroMax = 1600.0f;
    constexpr float kMinimumsRadioMax = 99.0f;
    constexpr float kBaroMinInHg = 26.0f;
    constexpr float kBaroMaxInHg = 31.5f;
    constexpr float kHpaPerInHg = 33.8639f;

    float stepInDirection(float value, bool up, float step) {
        return up ? std::floor(value / step) * step + step : std::ceil(value / step) * step - step;
    }

    bool isFastDelta(char delta) {
        return delta == 10 || delta == -10;
    }
}

FPSEjetsPDCProfile::FPSEjetsPDCProfile(ProductPDC *product) : PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kBacklight, [product](const std::vector<float> &brightness) {
        if (brightness.size() <= kBacklightIndex) {
            return;
        }

        product->setLedBrightness(PDCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness[kBacklightIndex], 0.0f, 1.0f) * 255));
        product->forceStateSync();
    },
        this);
}

bool FPSEjetsPDCProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

std::string FPSEjetsPDCProfile::sideDataref(const char *pilot, const char *copilot) const {
    return product->isCaptainSide() ? pilot : copilot;
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &FPSEjetsPDCProfile::buttonDefs() const {
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    const std::string side = product->isCaptainSide() ? "pilot" : "copilot";
    const std::string nd = "ssg/B748/ND/";
    const std::string baro = sideDataref(kBaroPilot, kBaroCopilot);
    const std::string hsiButton = sideDataref("SSG/MCP/hsi_nd_p_sw", "SSG/MCP/hsi_nd_cp_sw");
    const std::string rangeUp = sideDataref("FPS/EJET/MISC/MAP_RANGE_UP_P_COMM", "FPS/EJET/MISC/MAP_RANGE_UP_CP_COMM");
    const std::string rangeDown = sideDataref("FPS/EJET/MISC/MAP_RANGE_DN_P_COMM", "FPS/EJET/MISC/MAP_RANGE_DN_CP_COMM");

    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", ""}},
                        {{1, 1}, {"MTRS", ""}},
                        {{-1, 2}, {"3M VSD", nd + "show_VP_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{2, 3}, {"WXR", nd + "show_t_w_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{3, 4}, {"STA", nd + "show_VOR_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{4, 5}, {"WPT", nd + "show_waypoint_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{5, 6}, {"ARPT", nd + "show_airport_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{6, 7}, {"DATA", ""}},
                        {{7, 8}, {"POS", nd + "show_POS_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{8, 9}, {"TERR", nd + "show_t_w_" + side, PDCDatarefType::TOGGLE_VALUE, 2.0}},
                        {{9, 10}, {"LEFT VOR1", ""}},
                        {{10, 11}, {"LEFT OFF", ""}},
                        {{11, 12}, {"LEFT ADF1", ""}},
                        {{12, 13}, {"RIGHT VOR2", ""}},
                        {{13, 14}, {"RIGHT OFF", ""}},
                        {{14, 15}, {"RIGHT ADF2", ""}},
                        {{15, 16}, {"Mins RST", ""}},
                        {{16, 17}, {"VOR MAP CTR", hsiButton, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
                        {{17, 18}, {"RANGE TFC", nd + "show_TCAS_" + side, PDCDatarefType::TOGGLE_VALUE, 1.0}},
                        {{18, 19}, {"Baro STD", baro, PDCDatarefType::SET_VALUE, 29.92}},
                        {{-1, 20}, {"3M Range Minus", rangeDown, PDCDatarefType::EXECUTE_CMD_ONCE}},
                        {{-1, 21}, {"3M Range Plus", rangeUp, PDCDatarefType::EXECUTE_CMD_ONCE}},
                        {{21, 22}, {"Baro knob left fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
                        {{23, 24}, {"Mins RADIO", kRaBaroSel, PDCDatarefType::SET_VALUE, 0.0}},
                        {{24, 25}, {"Mins BARO", kRaBaroSel, PDCDatarefType::SET_VALUE, 1.0}},
                        {{25, 26}, {"Baro inHg", kBaroType, PDCDatarefType::SET_VALUE, 0.0}},
                        {{26, 27}, {"Baro HPA", kBaroType, PDCDatarefType::SET_VALUE, 1.0}},
                        {{27, 28}, {"Map APP", ""}},
                        {{28, 29}, {"Map VOR", ""}},
                        {{29, 30}, {"Map MAP", ""}},
                        {{30, 31}, {"Map PLN", ""}},
                        {{31, -1}, {"3N Map range 5", kRangeDetent, PDCDatarefType::SET_VALUE, 0.0}},
                        {{32, -1}, {"3N Map range 10", kRangeDetent, PDCDatarefType::SET_VALUE, 1.0}},
                        {{33, -1}, {"3N Map range 20", kRangeDetent, PDCDatarefType::SET_VALUE, 2.0}},
                        {{34, -1}, {"3N Map range 40", kRangeDetent, PDCDatarefType::SET_VALUE, 3.0}},
                        {{35, -1}, {"3N Map range 80", kRangeDetent, PDCDatarefType::SET_VALUE, 4.0}},
                        {{36, -1}, {"3N Map range 160", kRangeDetent, PDCDatarefType::SET_VALUE, 5.0}},
                        {{37, -1}, {"3N Map range 320", kRangeDetent, PDCDatarefType::SET_VALUE, 6.0}},
                        {{38, -1}, {"3N Map range 640", kRangeDetent, PDCDatarefType::SET_VALUE, 7.0}},
                        {{19, 32}, {"Mins knob left fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                        {{39, 33}, {"Mins knob left slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", "custom", PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},
                        {{42, 37}, {"Baro knob left slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", "custom", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void FPSEjetsPDCProfile::update() {
    float now = XPLMGetElapsedTime();

    char minimumsDelta = minimumsFastDelta != 0 ? minimumsFastDelta : minimumsSlowDelta;
    if (minimumsDelta != 0 && now - minimumsLastCommandTime >= 0.1f) {
        minimumsLastCommandTime = now;
        changeMinimums(minimumsDelta);
    }

    char baroDelta = baroFastDelta != 0 ? baroFastDelta : baroSlowDelta;
    if (baroDelta != 0 && now - baroLastCommandTime >= 0.1f) {
        baroLastCommandTime = now;
        changeBaro(baroDelta);
    }
}

void FPSEjetsPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING || button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        handleKnob(button, phase);
        return;
    }

    auto dm = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::SET_VALUE_MOMENTARY) {
        if (phase == xplm_CommandBegin) {
            dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
        } else if (phase == xplm_CommandEnd) {
            dm->set<float>(button->dataref.c_str(), 0.0f);
        }
        return;
    }

    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == kRangeDetent) {
        selectRangeDetent(static_cast<int>(button->value));
    } else if (button->datarefType == PDCDatarefType::TOGGLE_VALUE) {
        float current = dm->get<float>(button->dataref.c_str());
        bool isOn = std::lround(current) == std::lround(button->value);
        dm->set<float>(button->dataref.c_str(), isOn ? 0.0f : static_cast<float>(button->value));
    } else if (button->datarefType == PDCDatarefType::SET_VALUE) {
        dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    }
}

void FPSEjetsPDCProfile::handleKnob(const PDCButtonDef *button, XPLMCommandPhase phase) {
    bool isBaroKnob = button->datarefType == PDCDatarefType::ADD_BARO_REPEATING;
    char &slowDelta = isBaroKnob ? baroSlowDelta : minimumsSlowDelta;
    char &fastDelta = isBaroKnob ? baroFastDelta : minimumsFastDelta;
    float &fastReleaseTime = isBaroKnob ? baroFastReleaseTime : minimumsFastReleaseTime;
    float &lastCommandTime = isBaroKnob ? baroLastCommandTime : minimumsLastCommandTime;

    char value = static_cast<char>(button->value);
    bool isFast = isFastDelta(value);
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandEnd) {
        if (isFast) {
            fastDelta = 0;
            fastReleaseTime = now;
            lastCommandTime = now + 1.0f;
        } else {
            slowDelta = 0;
        }
        return;
    }

    if (isFast) {
        fastDelta = value;
        lastCommandTime = now + 1.0f;
        isBaroKnob ? changeBaro(value) : changeMinimums(value);
        return;
    }

    slowDelta = value;
    lastCommandTime = now + 1.0f;
    if (fastDelta == 0 && now - fastReleaseTime > 0.3f) {
        isBaroKnob ? changeBaro(value) : changeMinimums(value);
    }
}

void FPSEjetsPDCProfile::changeMinimums(char delta) {
    if (delta == 0) {
        return;
    }

    auto dm = Dataref::getInstance();
    bool isBaro = dm->get<float>(kRaBaroSel) >= 0.5f;
    const char *dataref = isBaro ? kMinimumsBaro : kMinimumsRadio;
    float maximum = isBaro ? kMinimumsBaroMax : kMinimumsRadioMax;

    float current = std::round(dm->get<float>(dataref));
    float target = isFastDelta(delta) ? stepInDirection(current, delta > 0, 10.0f) : current + delta;
    dm->set<float>(dataref, std::clamp(target, 0.0f, maximum));
}

void FPSEjetsPDCProfile::changeBaro(char delta) {
    if (delta == 0) {
        return;
    }

    auto dm = Dataref::getInstance();
    const std::string dataref = sideDataref(kBaroPilot, kBaroCopilot);
    bool isHpa = dm->get<float>(kBaroType) >= 0.5f;
    float baroInHg = dm->get<float>(dataref.c_str());

    bool up = delta > 0;
    float step = isFastDelta(delta) ? 10.0f : 1.0f;

    if (isHpa) {
        baroInHg = stepInDirection(std::round(baroInHg * kHpaPerInHg), up, step) / kHpaPerInHg;
    } else {
        baroInHg = stepInDirection(std::round(baroInHg * 100.0f), up, step) / 100.0f;
    }

    dm->set<float>(dataref.c_str(), std::clamp(baroInHg, kBaroMinInHg, kBaroMaxInHg));
}

void FPSEjetsPDCProfile::selectRangeDetent(int detent) {
    if (lastRangeDetent < 0) {
        lastRangeDetent = detent;
        return;
    }

    int steps = detent - lastRangeDetent;
    lastRangeDetent = detent;
    if (steps == 0) {
        return;
    }

    const std::string command = steps > 0
                                    ? sideDataref("FPS/EJET/MISC/MAP_RANGE_UP_P_COMM", "FPS/EJET/MISC/MAP_RANGE_UP_CP_COMM")
                                    : sideDataref("FPS/EJET/MISC/MAP_RANGE_DN_P_COMM", "FPS/EJET/MISC/MAP_RANGE_DN_CP_COMM");

    auto dm = Dataref::getInstance();
    for (int i = 0; i < std::abs(steps); i++) {
        dm->executeCommand(command.c_str());
    }
}

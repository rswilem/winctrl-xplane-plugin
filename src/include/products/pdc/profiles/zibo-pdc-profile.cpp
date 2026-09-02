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
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/dspl_light_test", [this](const std::vector<float> &displayTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/electric/main_bus", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
    },
        this);

    bool isCaptain = product->isCaptainSide();

    // Arm the standard-pressure seed while STD is engaged. Nothing is written
    // here: writing would light up the preselect on the PFD before the pilot
    // has touched the knob (issue #109).
    std::string stdDataref = std::string("laminar/B738/EFIS/baro_set_std_") + (isCaptain ? "pilot" : "copilot");
    Dataref::getInstance()->monitorExistingDataref<bool>(stdDataref.c_str(), [this](bool stdActive) {
        baroStdSeedPending = stdActive;
    },
        this);

    // Remember the last minimums the pilot actually dialled in, so the knob
    // resumes from it after RST parks the dataref at its sentinel.
    std::string minsDataref = std::string("laminar/B738/pfd/dh_") + (isCaptain ? "pilot" : "copilot");
    std::string minsModeDataref = std::string("laminar/B738/EFIS_control/") + (isCaptain ? "cpt" : "fo") + "/minimums";
    Dataref::getInstance()->monitorExistingDataref<float>(minsDataref.c_str(), [this, minsModeDataref](float mins) {
        if (mins <= 0.0f) {
            return;
        }

        if (Dataref::getInstance()->get<int>(minsModeDataref.c_str()) == 1) {
            lastMinimumsBaro = mins;
        } else {
            lastMinimumsRadio = mins;
        }
    },
        this);
}

bool ZiboPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &ZiboPDCProfile::buttonDefs() const {
    const std::string pilotSide = product->isCaptainSide() ? "capt" : "fo";
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
                        {{21, 22}, {"Baro knob left fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", "custom", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
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

void ZiboPDCProfile::update() {
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

void ZiboPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING || button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        bool isBaroKnob = button->datarefType == PDCDatarefType::ADD_BARO_REPEATING;
        char &slowDelta = isBaroKnob ? baroSlowDelta : minimumsSlowDelta;
        char &fastDelta = isBaroKnob ? baroFastDelta : minimumsFastDelta;
        float &fastReleaseTime = isBaroKnob ? baroFastReleaseTime : minimumsFastReleaseTime;
        float &lastCommandTime = isBaroKnob ? baroLastCommandTime : minimumsLastCommandTime;

        char value = static_cast<char>(button->value);
        bool isFast = value == 10 || value == -10;
        float now = XPLMGetElapsedTime();

        if (phase == xplm_CommandEnd) {
            if (isFast) {
                fastDelta = 0;
                fastReleaseTime = now;
                // A slow contact that lingers after the fast detent releases
                // must re-arm the hold delay instead of repeating ±1.
                lastCommandTime = now + 1.0f;
            } else {
                slowDelta = 0;
            }
        } else if (isFast) {
            fastDelta = value;
            lastCommandTime = now + 1.0f;
            isBaroKnob ? changeBaro(value) : changeMinimums(value);
        } else {
            slowDelta = value;
            lastCommandTime = now + 1.0f;
            // Suppress the immediate ±1 while the fast detent is engaged or
            // when passing through the slow position on release (issue #109).
            if (fastDelta == 0 && now - fastReleaseTime > 0.3f) {
                isBaroKnob ? changeBaro(value) : changeMinimums(value);
            }
        }
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

// Fast detent (|delta| == 10): snap to the next multiple of 10 in the turn
// direction, e.g. 1013 -> 1020/1010 (issue #109).
static float snapToTens(float value, bool up) {
    float snapped = up ? std::floor(value / 10.0f) * 10.0f + 10.0f : std::ceil(value / 10.0f) * 10.0f - 10.0f;
    return snapped;
}

void ZiboPDCProfile::changeMinimums(char delta) {
    if (delta == 0) {
        return;
    }

    bool isCaptain = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    auto datarefManager = Dataref::getInstance();
    // 0 = RADIO, 1 = BARO
    bool isBaro = datarefManager->get<int>((std::string("laminar/B738/EFIS_control/") + (isCaptain ? "cpt" : "fo") + "/minimums").c_str()) == 1;
    std::string dataref = std::string("laminar/B738/pfd/dh_") + (isCaptain ? "pilot" : "copilot");
    float currentMins = datarefManager->get<float>(dataref.c_str());

    // Zibo parks the value at a negative sentinel when the minimums are unset
    // or reset; continue from the last dialled value for this mode instead.
    if (currentMins <= 0.0f) {
        currentMins = isBaro ? lastMinimumsBaro : lastMinimumsRadio;
    }

    if (delta == 10 || delta == -10) {
        currentMins = snapToTens(currentMins, delta > 0);
    } else {
        currentMins += delta;
    }

    datarefManager->set<float>(dataref.c_str(), std::max(currentMins, 0.0f));
}

void ZiboPDCProfile::changeBaro(char delta) {
    if (delta == 0) {
        return;
    }

    bool isCaptain = product->deviceVariant == PDCDeviceVariant::VARIANT_3N_CAPTAIN || product->deviceVariant == PDCDeviceVariant::VARIANT_3M_CAPTAIN;
    auto datarefManager = Dataref::getInstance();
    bool isHPA = datarefManager->get<bool>((std::string("laminar/B738/EFIS_control/") + (isCaptain ? "capt" : "fo") + "/baro_in_hpa").c_str());
    std::string dataref = std::string("laminar/B738/EFIS/baro_sel_in_hg_") + (isCaptain ? "pilot" : "copilot");
    float currentBaroInHg = datarefManager->get<float>(dataref.c_str());

    // First turn after STD engaged starts from standard pressure, in whichever
    // unit the knob is set to (issue #109).
    bool seedStandard = baroStdSeedPending;
    baroStdSeedPending = false;

    bool isFast = delta == 10 || delta == -10;
    if (isHPA) {
        // Snap/step in whole hPa; the dataref itself stays in inHg.
        float hpa = seedStandard ? 1013.0f : std::round(currentBaroInHg / 0.02953f);
        hpa = isFast ? snapToTens(hpa, delta > 0) : hpa + delta;
        currentBaroInHg = hpa * 0.02953f;
    } else {
        // Slow: 0.01 inHg; fast: snap to the next 0.10 inHg.
        float centiInHg = seedStandard ? 2992.0f : std::round(currentBaroInHg * 100.0f);
        centiInHg = isFast ? snapToTens(centiInHg, delta > 0) : centiInHg + delta;
        currentBaroInHg = centiInHg / 100.0f;
    }

    datarefManager->set<float>(dataref.c_str(), currentBaroInHg);
}
#include "fps-ejets-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    constexpr const char *kPowerRef = "ssg/Elec/ac_alive";
    constexpr const char *kMainPanelRef = "SSG/EJET/LIGHTS/main_pnl_sw";
    constexpr const char *kGearDeployRef = "sim/aircraft/parts/acf_gear_deploy";
    constexpr const char *kAutobrakeRef = "SSG/GEAR/autobrake_on";
    constexpr const char *kTerrainPilotRef = "ssg/B748/ND/show_t_w_pilot";
    constexpr const char *kTerrainCopilotRef = "ssg/B748/ND/show_t_w_copilot";
    constexpr const char *kElapsedHourRef = "ssg/EJET/CLOCK/time_elapsed_hour";
    constexpr const char *kElapsedMinuteRef = "ssg/EJET/CLOCK/time_elapsed_min";
    constexpr const char *kDateRef = "ssg/EJET/CLOCK/d_t";
    constexpr float kAutobrakeOff = 1.0f;
    constexpr float kTerrainOff = 0.0f;
    constexpr float kTerrainOn = 2.0f;
}

FPSEjetsAGPProfile::FPSEjetsAGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    auto datarefManager = Dataref::getInstance();
    hasElapsedTime = datarefManager->exists(kElapsedHourRef) && datarefManager->exists(kElapsedMinuteRef);

    datarefManager->monitorExistingDataref<float>(kPowerRef, [this](float) {
        updateBacklight();
        updateLights();
    },
        this);

    datarefManager->monitorExistingDataref<float>(kMainPanelRef, [this](float) {
        updateBacklight();
    },
        this);

    datarefManager->monitorExistingDataref<std::vector<float>>(kGearDeployRef, [this](const std::vector<float> &) {
        updateLights();
    },
        this);

    datarefManager->monitorExistingDataref<float>(kAutobrakeRef, [this](float) {
        updateLights();
    },
        this);

    datarefManager->monitorExistingDataref<float>(kTerrainPilotRef, [this](float) {
        updateLights();
    },
        this);

    datarefManager->monitorExistingDataref<float>(kTerrainCopilotRef, [this](float) {
        updateLights();
    },
        this);
}

bool FPSEjetsAGPProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

const std::unordered_map<uint16_t, AGPButtonDef> &FPSEjetsAGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {0, {"Brake fan on", ""}},
        {1, {"Brake fan off", ""}},
        {2, {"Autobrake Lo", kAutobrakeRef, AGPDatarefType::SET_VALUE, 2}},
        {3, {"Autobrake Medium", kAutobrakeRef, AGPDatarefType::SET_VALUE, 3}},
        {4, {"Autobrake Max", kAutobrakeRef, AGPDatarefType::SET_VALUE, 4}},
        {5, {"Antiskid on", ""}},
        {6, {"Antiskid off", ""}},
        {7, {"RST Turn left", ""}},
        {8, {"RST Press", "ssg/EJET/CLOCK/reset", AGPDatarefType::SET_VALUE, 1}},
        {9, {"RST Turn right", ""}},
        {10, {"CHR Turn left", ""}},
        {11, {"CHR Press", "ssg/EJET/CLOCK/chrono_sw", AGPDatarefType::SET_VALUE, 1}},
        {12, {"CHR Turn right", ""}},
        {13, {"Date Turn left", ""}},
        {14, {"Date Press", kDateRef, AGPDatarefType::SET_VALUE, 1}},
        {15, {"Date Turn right", ""}},
        {16, {"UTC switch GPS", ""}},
        {17, {"UTC switch INT", ""}},
        {18, {"UTC switch SET", ""}},
        {19, {"ET switch RUN", ""}},
        {20, {"ET switch STP", ""}},
        {21, {"ET switch RST", ""}},
        {22, {"TERR ON ND", kTerrainPilotRef, AGPDatarefType::TERRAIN_ON_ND}},
        {23, {"GEAR UP", "sim/cockpit2/controls/gear_handle_down", AGPDatarefType::LANDING_GEAR, 0}},
        {24, {"GEAR DOWN", "sim/cockpit2/controls/gear_handle_down", AGPDatarefType::LANDING_GEAR, 1}},
    };

    return buttons;
}

void FPSEjetsAGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == AGPDatarefType::LANDING_GEAR) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
        return;
    }

    if (button->datarefType == AGPDatarefType::TERRAIN_ON_ND) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        if (product->terrainNDPreference == AGPTerrainNDPreference::BOTH) {
            bool currentlyOn = datarefManager->getCached<float>(kTerrainPilotRef) > 1.5f || datarefManager->getCached<float>(kTerrainCopilotRef) > 1.5f;
            float newValue = currentlyOn ? kTerrainOff : kTerrainOn;
            datarefManager->set<float>(kTerrainPilotRef, newValue);
            datarefManager->set<float>(kTerrainCopilotRef, newValue);
        } else {
            const char *ref = product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN ? kTerrainPilotRef : kTerrainCopilotRef;
            datarefManager->set<float>(ref, datarefManager->getCached<float>(ref) > 1.5f ? kTerrainOff : kTerrainOn);
        }
        return;
    }

    if (button->dataref == kAutobrakeRef) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        int position = static_cast<int>(std::lround(datarefManager->getCached<float>(kAutobrakeRef)));
        int target = static_cast<int>(button->value);
        datarefManager->set<float>(kAutobrakeRef, position == target ? kAutobrakeOff : static_cast<float>(target));
        return;
    }

    if (button->datarefType == AGPDatarefType::SET_VALUE) {
        bool pressed = phase == xplm_CommandBegin;
        datarefManager->set<float>(button->dataref.c_str(), pressed ? static_cast<float>(button->value) : 0.0f);

        if (button->dataref == kDateRef) {
            showDate = pressed;
            updateDisplays();
        }
        return;
    }

    datarefManager->executeCommand(button->dataref.c_str(), phase);
}

void FPSEjetsAGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    std::string chrono = "";
    std::string utc = "";
    std::string elapsed = "";

    if (hasPower()) {
        if (showDate) {
            utc = SegmentDisplay::formatDateFromDayOfYear(datarefManager->getCached<int>("sim/time/local_date_days") + 1);
        } else {
            utc = SegmentDisplay::formatTimeFromSecondsOfDay(datarefManager->getCached<double>("sim/time/zulu_time_sec"));
        }

        if (hasElapsedTime) {
            int hours = std::max(0, static_cast<int>(datarefManager->getCached<float>(kElapsedHourRef))) % 100;
            int minutes = std::max(0, static_cast<int>(datarefManager->getCached<float>(kElapsedMinuteRef))) % 60;
            elapsed = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(minutes), 2);
        }
    }

    product->setLCDText(chrono, utc, elapsed);
}

bool FPSEjetsAGPProfile::hasPower() {
    return Dataref::getInstance()->getCached<float>(kPowerRef) > 0.5f;
}

void FPSEjetsAGPProfile::updateBacklight() {
    if (!product) {
        return;
    }

    bool powered = hasPower();
    float panel = std::clamp(Dataref::getInstance()->getCached<float>(kMainPanelRef), 0.0f, 1.0f);

    product->setLedBrightness(AGPLed::BACKLIGHT, powered ? static_cast<uint8_t>(panel * 255) : 0);
    product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, powered ? 255 : 0);
    product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, powered ? 255 : 0);
}

bool FPSEjetsAGPProfile::terrainSelected() {
    auto datarefManager = Dataref::getInstance();
    bool pilot = datarefManager->getCached<float>(kTerrainPilotRef) > 1.5f;
    bool copilot = datarefManager->getCached<float>(kTerrainCopilotRef) > 1.5f;

    switch (product->terrainNDPreference) {
        case AGPTerrainNDPreference::CAPTAIN:
            return pilot;
        case AGPTerrainNDPreference::FIRST_OFFICER:
            return copilot;
        case AGPTerrainNDPreference::BOTH:
            return pilot || copilot;
    }

    return false;
}

void FPSEjetsAGPProfile::updateLights() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    bool powered = hasPower();

    std::vector<float> deploy = datarefManager->getCached<std::vector<float>>(kGearDeployRef);
    auto gearDown = [&deploy, powered](size_t index) {
        return powered && index < deploy.size() && deploy[index] > 0.99f;
    };
    auto gearInTransit = [&deploy, powered](size_t index) {
        return powered && index < deploy.size() && deploy[index] > 0.01f && deploy[index] <= 0.99f;
    };

    product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearDown(0) ? 1 : 0);
    product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_LEFT, gearDown(1) ? 1 : 0);
    product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_RIGHT, gearDown(2) ? 1 : 0);
    product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_CENTER, gearInTransit(0) ? 1 : 0);
    product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_LEFT, gearInTransit(1) ? 1 : 0);
    product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_RIGHT, gearInTransit(2) ? 1 : 0);

    int autobrake = static_cast<int>(std::lround(datarefManager->getCached<float>(kAutobrakeRef)));
    product->setLedBrightness(AGPLed::AUTOBRK_LO_ON, powered && autobrake == 2 ? 1 : 0);
    product->setLedBrightness(AGPLed::AUTOBRK_MED_ON, powered && autobrake == 3 ? 1 : 0);
    product->setLedBrightness(AGPLed::AUTOBRK_MAX_ON, powered && autobrake == 4 ? 1 : 0);

    product->setLedBrightness(AGPLed::TERRAIN_ON, powered && terrainSelected() ? 1 : 0);
}

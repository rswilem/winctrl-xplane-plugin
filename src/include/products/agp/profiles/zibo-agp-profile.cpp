#include "zibo-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

ZiboAGPProfile::ZiboAGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/electric/instrument_brightness", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t backlightBrightness = static_cast<uint8_t>(brightness * 255);
        product->setLedBrightness(AGPLed::BACKLIGHT, hasPower ? backlightBrightness : 0);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/instrument_brightness");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/left_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_LEFT, gearSafe > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/nose_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearSafe > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/right_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_RIGHT, gearSafe > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/left_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_LEFT, gearTransit > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/nose_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_CENTER, gearTransit > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/right_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_RIGHT, gearTransit > 0.0f ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autobrake/autobrake_pos", [product](float position) {
        int pos = static_cast<int>(std::lround(position));
        product->setLedBrightness(AGPLed::AUTOBRK_LO_ON, pos == 2 ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MED_ON, (pos == 3 || pos == 4) ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MAX_ON, (pos == 0 || pos == 5) ? 1 : 0);
    },
        this);

    auto terrainLight = [product](float) {
        auto datarefManager = Dataref::getInstance();
        bool on = datarefManager->get<float>("laminar/B738/EFIS/EFIS_wxr_terr") > 0.5f ||
                  datarefManager->get<float>("laminar/B738/EFIS/EFIS_fo_wxr_terr") > 0.5f;
        product->setLedBrightness(AGPLed::TERRAIN_ON, on ? 1 : 0);
    };
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/EFIS/EFIS_wxr_terr", terrainLight, this);
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/EFIS/EFIS_fo_wxr_terr", terrainLight, this);
}

bool ZiboAGPProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::unordered_map<uint16_t, AGPButtonDef> &ZiboAGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {0, {"Brake fan on", ""}},
        {1, {"Brake fan off", ""}},
        {2, {"Autobrake Lo", "zibo_autobrake_lo"}},
        {3, {"Autobrake Medium", "zibo_autobrake_med"}},
        {4, {"Autobrake Max", "zibo_autobrake_max"}},
        {5, {"Antiskid on", ""}},
        {6, {"Antiskid off", ""}},
        {7, {"RST Turn left", ""}},
        {8, {"RST Press", ""}},
        {9, {"RST Turn right", ""}},
        {10, {"CHR Turn left", ""}},
        {11, {"CHR Press", "laminar/B738/push_button/chrono_cycle_capt"}},
        {12, {"CHR Turn right", ""}},
        {13, {"Date Turn left", ""}},
        {14, {"Date Press", "zibo_clock_date"}},
        {15, {"Date Turn right", ""}},
        {16, {"UTC switch GPS", ""}},
        {17, {"UTC switch INT", ""}},
        {18, {"UTC switch SET", ""}},
        {19, {"ET switch RUN", "zibo_et_run"}},
        {20, {"ET switch STP", "zibo_et_stop"}},
        {21, {"ET switch RST", "laminar/B738/push_button/et_reset_capt"}},
        {22, {"TERR ON ND", "zibo_terrain"}},
        {23, {"GEAR UP", "laminar/B738/push_button/gear_up"}},
        {24, {"GEAR DOWN", "laminar/B738/push_button/gear_down"}},
    };

    return buttons;
}

void ZiboAGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->dataref == "zibo_terrain") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        if (product->terrainNDPreference == AGPTerrainNDPreference::BOTH) {
            bool captainOn = datarefManager->get<float>("laminar/B738/EFIS/EFIS_wxr_terr") > 0.5f;
            bool firstOfficerOn = datarefManager->get<float>("laminar/B738/EFIS/EFIS_fo_wxr_terr") > 0.5f;
            bool wantOn = !(captainOn || firstOfficerOn);
            if (captainOn != wantOn) {
                datarefManager->executeCommand("laminar/B738/EFIS_control/capt/push_button/terr_press");
            }
            if (firstOfficerOn != wantOn) {
                datarefManager->executeCommand("laminar/B738/EFIS_control/fo/push_button/terr_press");
            }
        } else {
            std::string command = (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) ? "laminar/B738/EFIS_control/capt/push_button/terr_press" : "laminar/B738/EFIS_control/fo/push_button/terr_press";
            datarefManager->executeCommand(command.c_str());
        }
        return;
    }

    if (button->dataref == "zibo_autobrake_lo" || button->dataref == "zibo_autobrake_med" || button->dataref == "zibo_autobrake_max") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        int pos = static_cast<int>(std::lround(datarefManager->get<float>("laminar/B738/autobrake/autobrake_pos")));
        const char *command;
        if (button->dataref == "zibo_autobrake_lo") {
            command = pos == 2 ? "laminar/B738/knob/autobrake_off" : "laminar/B738/knob/autobrake_1";
        } else if (button->dataref == "zibo_autobrake_med") {
            command = (pos == 3 || pos == 4) ? "laminar/B738/knob/autobrake_off" : "laminar/B738/knob/autobrake_2";
        } else {
            command = (pos == 0 || pos == 5) ? "laminar/B738/knob/autobrake_off" : "laminar/B738/knob/autobrake_max";
        }
        datarefManager->executeCommand(command);
        return;
    }

    if (button->dataref == "zibo_clock_date") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        int mode = static_cast<int>(std::lround(datarefManager->get<float>("laminar/B738/clock/clock_display_mode_capt")));
        if (mode < 1 || mode > 4) {
            return;
        }
        clockTarget = mode == 1 ? 2 : mode == 2 ? 1 : mode == 3 ? 4 : 3;
        clockPressesLeft = 4;
        stepClock();
        return;
    }

    if (button->dataref == "zibo_et_run" || button->dataref == "zibo_et_stop") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        bool wantRun = button->dataref == "zibo_et_run";
        bool running = datarefManager->get<float>("laminar/B738/clock/captain/et_mode") > 0.5f;
        if (running != wantRun) {
            datarefManager->executeCommand("laminar/B738/push_button/chrono_capt_et_mode");
        }
        return;
    }

    if (button->dataref.empty()) {
        return;
    }

    if (button->datarefType == AGPDatarefType::LANDING_GEAR) {
        // The lever is a maintained switch: holding the command down would keep
        // asserting the position every frame and override external changes.
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->executeCommand(button->dataref.c_str(), -1);
    } else if (button->datarefType == AGPDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else if (button->datarefType == AGPDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboAGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    stepClock();

    auto datarefManager = Dataref::getInstance();
    std::string chrono = "";
    std::string utc = "";
    std::string elapsed = "";

    if (datarefManager->get<bool>("sim/cockpit/electrical/battery_on")) {
        if (datarefManager->get<float>("laminar/B738/clock/captain/chrono_mode") > -0.5f) {
            int minutes = std::max(0, static_cast<int>(datarefManager->get<float>("laminar/B738/clock/captain/chrono_minutes"))) % 100;
            int seconds = std::max(0, static_cast<int>(datarefManager->get<float>("laminar/B738/clock/captain/chrono_seconds"))) % 60;
            chrono = SegmentDisplay::fixStringLength(std::to_string(minutes), 2) + ":" +
                     SegmentDisplay::fixStringLength(std::to_string(seconds), 2);
        }

        int clockMode = static_cast<int>(std::lround(datarefManager->get<float>("laminar/B738/clock/clock_display_mode_capt")));
        if (clockMode == 2 || clockMode == 4) {
            utc = SegmentDisplay::formatDateFromDayOfYear(datarefManager->get<int>("sim/time/local_date_days") + 1);
        } else {
            utc = SegmentDisplay::formatTimeFromSecondsOfDay(datarefManager->get<double>("sim/time/zulu_time_sec"));
        }

        if (datarefManager->get<float>("laminar/B738/clock/captain/et_mode") > -0.5f) {
            int hours = std::max(0, static_cast<int>(datarefManager->get<float>("laminar/B738/clock/captain/et_hours"))) % 100;
            int minutes = std::max(0, static_cast<int>(datarefManager->get<float>("laminar/B738/clock/captain/et_minutes"))) % 60;
            elapsed = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(minutes), 2);
        }
    }

    product->setLCDText(chrono, utc, elapsed);
}

void ZiboAGPProfile::stepClock() {
    if (clockTarget == 0) {
        return;
    }

    int mode = static_cast<int>(std::lround(Dataref::getInstance()->get<float>("laminar/B738/clock/clock_display_mode_capt")));
    if (mode == clockTarget || clockPressesLeft <= 0) {
        clockTarget = 0;
        return;
    }

    clockPressesLeft--;
    Dataref::getInstance()->executeCommand("laminar/B738/push_button/chrono_disp_mode_capt");
}

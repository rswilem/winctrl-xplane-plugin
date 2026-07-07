#include "toliss-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"
#include "xplane-version.hpp"

#include <algorithm>
#include <cmath>

TolissAGPProfile::TolissAGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasEssentialBusPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(AGPLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this, product](int annunMode) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/TerrainSelectedND1");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/TerrainSelectedND2");
        Dataref::getInstance()->executeChangedCallbacksForDataref(ifXPlane11("AirbusFBW/OHPLightsATA32", "AirbusFBW/OHPLightsATA32_Raw"));

        product->setLedBrightness(AGPLed::LDG_GEAR_LEVER_RED, isAnnunTest() ? 255 : 0);
        updateDisplays();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/TerrainSelectedND1", [this, product](bool enabled) {
        if (product->terrainNDPreference == AGPTerrainNDPreference::FIRST_OFFICER) {
            return;
        }

        bool terrainOn = (product->terrainNDPreference == AGPTerrainNDPreference::BOTH)
                             ? enabled || Dataref::getInstance()->get<bool>("AirbusFBW/TerrainSelectedND2")
                             : enabled;
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        product->setLedBrightness(AGPLed::TERRAIN_ON, hasPower && (terrainOn || isAnnunTest()) ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/TerrainSelectedND2", [this, product](bool enabled) {
        if (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) {
            return;
        }

        bool terrainOn = (product->terrainNDPreference == AGPTerrainNDPreference::BOTH)
                             ? enabled || Dataref::getInstance()->get<bool>("AirbusFBW/TerrainSelectedND1")
                             : enabled;
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        product->setLedBrightness(AGPLed::TERRAIN_ON, hasPower && (terrainOn || isAnnunTest()) ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(ifXPlane11("AirbusFBW/OHPLightsATA32", "AirbusFBW/OHPLightsATA32_Raw"), [this, product](const std::vector<float> &panelLights) {
        if (panelLights.size() < 18) {
            return;
        }

        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, panelLights[0] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_CENTER, panelLights[1] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_LEFT, panelLights[2] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_LEFT, panelLights[3] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_RIGHT, panelLights[4] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_RIGHT, panelLights[5] > std::numeric_limits<float>::epsilon() ? 1 : 0);

        product->setLedBrightness(AGPLed::BRAKE_FAN_ON, panelLights[10] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::BRAKE_FAN_HOT, panelLights[11] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_LO_ON, panelLights[12] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_LO_DECEL, panelLights[13] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MED_ON, panelLights[14] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MED_DECEL, panelLights[15] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MAX_ON, panelLights[16] > std::numeric_limits<float>::epsilon() ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MAX_DECEL, panelLights[17] > std::numeric_limits<float>::epsilon() ? 1 : 0);
    },
        this);
}

bool TolissAGPProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::unordered_map<uint16_t, AGPButtonDef> &TolissAGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {0, {"Brake fan on", "AirbusFBW/BrakeFan", AGPDatarefType::SET_VALUE, 1}},
        {1, {"Brake fan off", "AirbusFBW/BrakeFan", AGPDatarefType::SET_VALUE, 0}},
        {2, {"Autobrake Lo", "AirbusFBW/AbrkLo"}},
        {3, {"Autobrake Medium", "AirbusFBW/AbrkMed"}},
        {4, {"Autobrake Max", "AirbusFBW/AbrkMax"}},
        {5, {"Antiskid on", "AirbusFBW/NWSnAntiSkid", AGPDatarefType::SET_VALUE, 1}},
        {6, {"Antiskid off", "AirbusFBW/NWSnAntiSkid", AGPDatarefType::SET_VALUE, 0}},
        {7, {"RST Turn left", ""}},
        {8, {"RST Press", "toliss_airbus/chrono/ChronoResetPush"}},
        {9, {"RST Turn right", ""}},
        {10, {"CHR Turn left", ""}},
        {11, {"CHR Press", "toliss_airbus/chrono/ChronoStartStopPush"}},
        {12, {"CHR Turn right", ""}},
        {13, {"Date Turn left", ""}},
        {14, {"Date Press", "toliss_airbus/chrono/datePush"}},
        {15, {"Date Turn right", ""}},
        {16, {"UTC switch GPS", "ckpt/clock/gpsKnob/anim", AGPDatarefType::SET_VALUE, 0}},
        {17, {"UTC switch INT", "ckpt/clock/gpsKnob/anim", AGPDatarefType::SET_VALUE, 1}},
        {18, {"UTC switch SET", "ckpt/clock/gpsKnob/anim", AGPDatarefType::SET_VALUE, 2}},
        {19, {"ET switch RUN", "AirbusFBW/ClockETSwitch", AGPDatarefType::SET_VALUE, 0}},
        {20, {"ET switch STP", "AirbusFBW/ClockETSwitch", AGPDatarefType::SET_VALUE, 1}},
        {21, {"ET switch RST", "AirbusFBW/ClockETSwitch", AGPDatarefType::SET_VALUE, 2}},
        {22, {"TERR ON ND", "AirbusFBW/TerrainSelectedND1", AGPDatarefType::TERRAIN_ON_ND}},
        {23, {"GEAR UP", "sim/flight_controls/landing_gear_up", AGPDatarefType::LANDING_GEAR, 0}},
        {24, {"GEAR DOWN", "sim/flight_controls/landing_gear_down", AGPDatarefType::LANDING_GEAR, 1}},
    };

    return buttons;
}

void TolissAGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == AGPDatarefType::LANDING_GEAR) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
        datarefManager->set<int>("ckpt/gearHandle", static_cast<int>(button->value));
    } else if (button->datarefType == AGPDatarefType::TERRAIN_ON_ND) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        if (product->terrainNDPreference == AGPTerrainNDPreference::BOTH) {
            bool currentlyOn = datarefManager->get<bool>("AirbusFBW/TerrainSelectedND1") || datarefManager->get<bool>("AirbusFBW/TerrainSelectedND2");
            int newValue = currentlyOn ? 0 : 1;
            datarefManager->set<int>("AirbusFBW/TerrainSelectedND1", newValue);
            datarefManager->set<int>("AirbusFBW/TerrainSelectedND2", newValue);
        } else {
            std::string dataref = (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) ? "AirbusFBW/TerrainSelectedND1" : "AirbusFBW/TerrainSelectedND2";
            datarefManager->set<int>(dataref.c_str(), datarefManager->get<bool>(dataref.c_str()) ? 0 : 1);
        }
    } else if (button->datarefType == AGPDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void TolissAGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    std::string chrono = "";
    float chronoSeconds = datarefManager->get<float>("AirbusFBW/ClockChronoValue");
    if (chronoSeconds > std::numeric_limits<float>::epsilon()) {
        int totalSeconds = static_cast<int>(std::floor(chronoSeconds));
        int mins = totalSeconds / 60;
        int secs = totalSeconds % 60;
        chrono = SegmentDisplay::fixStringLength(std::to_string(mins), 2) + ":" +
                 SegmentDisplay::fixStringLength(std::to_string(secs), 2);
    }

    std::string utc = "";
    std::vector<float> buttonAnimations = datarefManager->get<std::vector<float>>("AirbusFBW/ChronoButtonAnimations");
    bool dateButtonPressed = buttonAnimations.size() > 2 && buttonAnimations[2] > std::numeric_limits<float>::epsilon();
    if (dateButtonPressed) {
        int dayOfYear = datarefManager->get<int>("sim/time/local_date_days") + 1;

        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        struct tm *timeinfo = std::localtime(&time);
        int year = timeinfo->tm_year + 1900;

        // Calculate month and day from day of year
        int month = 1;
        int day = dayOfYear;
        int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            daysInMonth[1] = 29;
        }

        for (int i = 0; i < 12; i++) {
            if (day <= daysInMonth[i]) {
                month = i + 1;
                break;
            }
            day -= daysInMonth[i];
        }

        // Format the date as MMDDYY
        utc = (month < 10 ? "0" : "") + std::to_string(month) + ":" +
              (day < 10 ? "0" : "") + std::to_string(day) + ":" +
              std::to_string(year % 100);
    } else {
        double zuluTime = datarefManager->get<double>("sim/time/zulu_time_sec");

        // Convert zulu time in seconds to HH:MM:SS
        int hours = static_cast<int>(zuluTime / 3600) % 24;
        int minutes = static_cast<int>(zuluTime / 60) % 60;
        int seconds = static_cast<int>(zuluTime) % 60;

        utc = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
              SegmentDisplay::fixStringLength(std::to_string(minutes), 2) + ":" +
              SegmentDisplay::fixStringLength(std::to_string(seconds), 2);
    }

    std::string elapsedTime = "";
    if (datarefManager->get<bool>("AirbusFBW/ClockShowsET")) {
        int hours = datarefManager->get<int>("AirbusFBW/ClockETHours");
        int minutes = datarefManager->get<int>("AirbusFBW/ClockETMinutes");
        elapsedTime = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(minutes), 2);
    }

    if (isAnnunTest(true)) {
        chrono = "88:88";
        utc = "88:88:88";
        elapsedTime = "88:88";
    }

    product->setLCDText(chrono, utc, elapsedTime);
}

bool TolissAGPProfile::isAnnunTest(bool allowEssentialBusPowerOnly) {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2 && (allowEssentialBusPowerOnly ? Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail") : Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on"));
}

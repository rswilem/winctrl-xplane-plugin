#include "pa28-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"
#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>

PA28AGPProfile::PA28AGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    // Same lighting approach as the PA28 FCU profile: LCD and LEDs on with battery power,
    // key backlight follows the effective panel brightness, only writing on actual change
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio", [this, product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        float panel = 0.0f;
        for (size_t i = 0; i < brightness.size() && i < 4; i++) {
            panel = std::max(panel, brightness[i]);
        }

        uint8_t backlight = hasPower ? panel * 255 : 0;
        uint8_t screens = hasPower ? 255 : 0;

        if (backlight == lastBacklightSent && screens == lastScreensSent) {
            return;
        }
        lastBacklightSent = backlight;
        lastScreensSent = screens;

        product->setLedBrightness(AGPLed::BACKLIGHT, backlight);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, screens);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Logger::getInstance()->info("AGP: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28AGPProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::unordered_map<uint16_t, AGPButtonDef> &PA28AGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {8, {"RST Press", "sim/instruments/timer_reset"}},
        {11, {"CHR Press", "sim/instruments/timer_start_stop"}},
        {14, {"Date Press", "custom_date"}},
        {19, {"ET switch RUN", "custom_et_run"}},
        {20, {"ET switch STP", "custom_et_stop"}},
        {21, {"ET switch RST", "custom_et_reset"}},
    };

    return buttons;
}

void PA28AGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->dataref == "custom_date") {
        // Show the date while the button is held
        showDate = phase == xplm_CommandBegin;
        updateDisplays();
        return;
    }

    if (button->dataref == "custom_et_run" || button->dataref == "custom_et_stop" || button->dataref == "custom_et_reset") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        if (button->dataref == "custom_et_run") {
            etRunning = true;
        } else if (button->dataref == "custom_et_stop") {
            etRunning = false;
        } else {
            etRunning = false;
            etAccumulatedSec = 0.0;
        }

        updateDisplays();
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

void PA28AGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // The elapsed-time counter follows sim time, so it pauses with the sim and
    // survives the switch positions like the real RUN/STP/RST clock switch
    double flightTime = datarefManager->get<float>("sim/time/total_flight_time_sec");
    if (etLastFlightTime >= 0 && etRunning && flightTime > etLastFlightTime) {
        etAccumulatedSec += flightTime - etLastFlightTime;
    }
    etLastFlightTime = flightTime;

    std::string chrono = "";
    float chronoSeconds = datarefManager->get<float>("sim/time/timer_elapsed_time_sec");
    bool chronoRunning = datarefManager->get<bool>("sim/time/timer_is_running_sec");
    if (chronoRunning || chronoSeconds > std::numeric_limits<float>::epsilon()) {
        int totalSeconds = static_cast<int>(std::floor(chronoSeconds));
        int mins = totalSeconds / 60;
        int secs = totalSeconds % 60;
        chrono = SegmentDisplay::fixStringLength(std::to_string(mins), 2) + ":" +
                 SegmentDisplay::fixStringLength(std::to_string(secs), 2);
    }

    std::string utc = "";
    if (showDate) {
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

        utc = (month < 10 ? "0" : "") + std::to_string(month) + ":" +
              (day < 10 ? "0" : "") + std::to_string(day) + ":" +
              std::to_string(year % 100);
    } else {
        double zuluTime = datarefManager->get<double>("sim/time/zulu_time_sec");

        int hours = static_cast<int>(zuluTime / 3600) % 24;
        int minutes = static_cast<int>(zuluTime / 60) % 60;
        int seconds = static_cast<int>(zuluTime) % 60;

        utc = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
              SegmentDisplay::fixStringLength(std::to_string(minutes), 2) + ":" +
              SegmentDisplay::fixStringLength(std::to_string(seconds), 2);
    }

    std::string elapsedTime = "";
    if (etRunning || etAccumulatedSec >= 1.0) {
        int etHours = static_cast<int>(etAccumulatedSec) / 3600;
        int etMinutes = (static_cast<int>(etAccumulatedSec) / 60) % 60;
        elapsedTime = SegmentDisplay::fixStringLength(std::to_string(etHours), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(etMinutes), 2);
    }

    if (!datarefManager->get<bool>("sim/cockpit/electrical/battery_on")) {
        chrono = "";
        utc = "";
        elapsedTime = "";
    }

    product->setLCDText(chrono, utc, elapsedTime);
}

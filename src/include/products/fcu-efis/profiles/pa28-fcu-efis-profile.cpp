#include "pa28-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"
#include "logger.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <XPLMUtilities.h>

PA28FCUEfisProfile::PA28FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // The JF PA28 animates the effective brightness array, not the _manual rheostat array.
    // Screens stay readable whenever the bus is powered; key backlights follow the panel lights.
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio", [this, product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        float panel = 0.0f;
        for (size_t i = 0; i < brightness.size() && i < 4; i++) {
            panel = std::max(panel, brightness[i]);
        }

        uint8_t backlight = hasPower ? panel * 255 : 0;
        uint8_t screens = hasPower ? 255 : 0;

        // The effective brightness array changes with ambient light (e.g. view switches);
        // only touch the hardware when the resulting levels actually differ
        if (backlight == lastBacklightSent && screens == lastScreensSent) {
            return;
        }
        lastBacklightSent = backlight;
        lastScreensSent = screens;

        product->setLedBrightness(FCUEfisLed::BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, 0);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, screens);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, screens);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, screens);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screens);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screens);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screens);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [product](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/servos_on", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, isAutopilotEngaged ? 1 : 0);
    },
        this);

    Logger::getInstance()->info("FCU-EFIS: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28FCUEfisProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");

    // Matches all PA-28 variants: P28A (Warrior/Archer/Cherokee), P28R (Arrow), P28T/P28U (Turbo Arrow)
    return icao.starts_with("P28");
}

const std::vector<std::string> &PA28FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &PA28FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {8, {"APPR", "sim/autopilot/approach"}},

        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/heading_sync"}},

        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {19, {"ALT PUSH", "sim/autopilot/altitude_hold"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_hold"}},
        {25, {"ALT 100", "custom_alt_step", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 100.0}},
        {26, {"ALT 1000", "custom_alt_step", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1000.0}},

        {39, {"L_STD PUSH", "custom_std_push"}},
        {40, {"L_STD PULL", "custom_std_pull"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "custom_unit_capt_inhg"}},
        {44, {"L_hPa", "custom_unit_capt_hpa"}},

        // Right EFIS controls the same single altimeter, with its own display unit
        {71, {"R_STD PUSH", "custom_std_push"}},
        {72, {"R_STD PULL", "custom_std_pull"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "custom_unit_fo_inhg"}},
        {76, {"R_hPa", "custom_unit_fo_hpa"}},
    };

    return buttons;
}

void PA28FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    data.displayTest = false;

    // No speed target on the PA-28: dashes with the managed dot, like an Airbus in managed speed
    data.speed = "---";
    data.spdManaged = true;
    data.spdMach = false;

    // Heading window shows the directional gyro heading bug
    float heading = datarefManager->getCached<float>("sim/cockpit/autopilot/heading_mag");
    int hdgDisplay = static_cast<int>(std::round(heading)) % 360;
    std::stringstream hdgSs;
    hdgSs << std::setfill('0') << std::setw(3) << hdgDisplay;
    data.heading = hdgSs.str();
    data.headingHdg = true;
    data.headingTrk = false;
    data.headingLat = true;
    data.hdgManaged = false;

    // Altitude window shows the autopilot altitude selector
    float altitude = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
    int altDisplay = std::max(0, static_cast<int>(std::round(altitude)));
    std::stringstream altSs;
    altSs << std::setfill('0') << std::setw(5) << altDisplay;
    data.altitude = altSs.str();
    data.altManaged = false;

    // No vertical speed target: dashed, Airbus style
    data.verticalSpeed = "-----";
    data.vsSign = false;
    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    data.fpaIndication = false;
    data.fpaComma = false;
    data.vsVerticalLine = false;

    // Both EFIS displays show QNH from the single altimeter, or Std, each in its own unit
    data.efisLeft.displayEnabled = data.displayEnabled;
    data.efisRight.displayEnabled = data.displayEnabled;

    if (stdMode) {
        data.efisLeft.isStd = true;
        data.efisLeft.unitIsInHg = false;
        data.efisLeft.baro = "";
        data.efisRight.isStd = true;
        data.efisRight.unitIsInHg = false;
        data.efisRight.baro = "";
    } else {
        float baroPilot = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
        data.efisLeft.setBaro(baroPilot, !captainUnitIsHpa);
        data.efisRight.setBaro(baroPilot, !foUnitIsHpa);
    }
}

void PA28FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    const char *baroRef = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";

    if (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO) {
        if (stdMode) {
            return;
        }

        bool unitIsHpa = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT ? captainUnitIsHpa : foUnitIsHpa;
        float baroValue = datarefManager->getCached<float>(baroRef);
        bool increase = button->value > 0;

        if (unitIsHpa) {
            float hpaValue = std::round(baroValue * 33.8639f) + (increase ? 1.0f : -1.0f);
            baroValue = hpaValue / 33.8639f;
        } else {
            baroValue += increase ? 0.01f : -0.01f;
        }

        datarefManager->set<float>(baroRef, baroValue);
    } else if (button->dataref == "custom_std_push") {
        if (!stdMode) {
            qnhBeforeStd = datarefManager->getCached<float>(baroRef);
            stdMode = true;
            datarefManager->set<float>(baroRef, 29.92f);
            product->updateDisplays();
        }
    } else if (button->dataref == "custom_std_pull") {
        if (stdMode) {
            stdMode = false;

            if (qnhBeforeStd > 0) {
                datarefManager->set<float>(baroRef, qnhBeforeStd);
            }

            product->updateDisplays();
        }
    } else if (button->dataref == "custom_altitude") {
        float altitude = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
        altitude += button->value > 0 ? altitudeIncrement : -altitudeIncrement;
        altitude = std::clamp(altitude, 0.0f, 50000.0f);
        datarefManager->set<float>("sim/cockpit/autopilot/altitude", altitude);
    } else if (button->dataref == "custom_alt_step") {
        altitudeIncrement = button->value;
    } else if (button->dataref == "custom_unit_capt_inhg" || button->dataref == "custom_unit_capt_hpa") {
        captainUnitIsHpa = button->dataref == "custom_unit_capt_hpa";
        product->updateDisplays();
    } else if (button->dataref == "custom_unit_fo_inhg" || button->dataref == "custom_unit_fo_hpa") {
        foUnitIsHpa = button->dataref == "custom_unit_fo_hpa";
        product->updateDisplays();
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

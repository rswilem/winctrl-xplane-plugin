#include "xcrafts-erj-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

XCraftsErjFCUEfisProfile::XCraftsErjFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 12) {
            return;
        }
        uint8_t target = static_cast<uint8_t>(brightness[12] * 255);
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, 255);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 255);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, 255);

        uint8_t screenBrightness = static_cast<uint8_t>(brightness[10] * 255);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit/autopilot/autopilot_mode", [product](int mode) {
        bool engaged = (mode == 2);
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/flight_director_mode", [product](int fdMode) {
        bool engaged = (fdMode == 1);
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, engaged ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/flight_director2_mode", [product](int fdMode) {
        bool engaged = (fdMode == 1);
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, engaged ? 1 : 0);
    }, this);
}

bool XCraftsErjFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/ERJ/MFD1/WX_TERR_status");
}

const std::vector<std::string> &XCraftsErjFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/autopilot/airspeed_is_mach",
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "sim/physics/metric_press",
        "sim/cockpit2/autopilot/flight_director_mode",
        "sim/cockpit2/autopilot/flight_director2_mode",
        "sim/cockpit2/EFIS/map_range",
        "sim/cockpit2/EFIS/map_range_copilot",
        "XCrafts/ERJ/MFD1/map_plan_status",
        "XCrafts/ERJ/MFD2/map_plan_status",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &XCraftsErjFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "sim/autopilot/NAV"}},
        {2, {"APPR", "sim/autopilot/approach"}},
        {3, {"AP", "sim/autopilot/servos_toggle"}},
        {5, {"A/THR", "sim/autopilot/autothrottle_toggle"}},

        // Speed encoder
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},

        // Heading encoder
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},

        // Altitude encoder
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},

        // Vertical speed encoder
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},

        // EFIS Left buttons
        {32, {"L_FD", "sim/autopilot/fdir_toggle"}},
        {39, {"L_STD", "sim/instruments/barometer_set_std"}},
        {40, {"L_STD PULL", "sim/instruments/barometer_set_std"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {45, {"L_MODE LS", "map_plan_mode", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {46, {"L_MODE VOR", "map_plan_mode", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {47, {"L_MODE NAV", "map_plan_mode"}},
        {48, {"L_MODE ARC", "map_plan_mode", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {49, {"L_MODE PLAN", "map_plan_mode", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {50, {"L_RANGE 10", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},

        // EFIS Right buttons
        {64, {"R_FD", "sim/autopilot/fdir2_toggle"}},
        {71, {"R_STD", "sim/instruments/barometer_set_std"}},
        {72, {"R_STD PULL", "sim/instruments/barometer_set_std"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {77, {"R_MODE LS", "map_plan_mode_copilot", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {78, {"R_MODE VOR", "map_plan_mode_copilot", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {79, {"R_MODE NAV", "map_plan_mode_copilot"}},
        {80, {"R_MODE ARC", "map_plan_mode_copilot", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 0.0}},
        {81, {"R_MODE PLAN", "map_plan_mode_copilot", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 5.0}},
    };
    return buttons;
}

void XCraftsErjFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.headingHdg = true;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::LevelChangeHeader;
    data.spdMach = datarefManager->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = datarefManager->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

    if (speed > 0) {
        std::stringstream ss;
        if (data.spdMach) {
            int machHundredths = static_cast<int>(std::round(speed * 100));
            ss << std::setfill('0') << std::setw(3) << machHundredths;
        } else {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(speed);
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }

    float heading = datarefManager->getCached<float>("sim/cockpit/autopilot/heading_mag");
    if (heading >= 0) {
        int hdgDisplay = static_cast<int>(heading) % 360;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }

    float altitude = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
    if (altitude >= 0) {
        int altInt = static_cast<int>(altitude);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    float vs = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    std::stringstream ss;
    int vsInt = static_cast<int>(std::round(vs));
    int absVs = std::abs(vsInt);

    if (absVs % 100 == 0) {
        ss << std::setfill('0') << std::setw(2) << (absVs / 100) << "##";
    } else {
        ss << std::setfill('0') << std::setw(4) << absVs;
    }
    data.verticalSpeed = ss.str();
    data.vsSign = (vs >= 0);
    data.vsIndication = true;

    data.headingLat = true;

    bool metricPress = datarefManager->getCached<bool>("sim/physics/metric_press");

    bool isStdPilot = datarefManager->getCached<bool>("sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot");
    float baroValuePilot = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    EfisDisplayValue pilotValue = {
        .baro = "",
        .unitIsInHg = false,
        .isStd = isStdPilot,
    };
    if (!isStdPilot && baroValuePilot > 0) {
        pilotValue.setBaro(baroValuePilot, !metricPress);
    }

    bool isStdCopilot = datarefManager->getCached<bool>("sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot");
    float baroValueCopilot = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");
    EfisDisplayValue copilotValue = {
        .baro = "",
        .unitIsInHg = false,
        .isStd = isStdCopilot,
    };
    if (!isStdCopilot && baroValueCopilot > 0) {
        copilotValue.setBaro(baroValueCopilot, !metricPress);
    }

    data.efisLeft = pilotValue;
    data.efisRight = copilotValue;
}

void XCraftsErjFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool metricPress = datarefManager->getCached<bool>("sim/physics/metric_press");
        bool increase = button->value > 0;

        if (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT) {
            bool isStd = datarefManager->getCached<bool>("sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot");
            if (isStd) {
                return;
            }
            float baroValue = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
            if (metricPress) {
                float hpaValue = baroValue * 33.8639f;
                hpaValue += increase ? 1.0f : -1.0f;
                baroValue = hpaValue / 33.8639f;
            } else {
                baroValue += increase ? 0.01f : -0.01f;
            }
            datarefManager->set<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot", baroValue);
        } else {
            bool isStd = datarefManager->getCached<bool>("sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot");
            if (isStd) {
                return;
            }
            float baroValue = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");
            if (metricPress) {
                float hpaValue = baroValue * 33.8639f;
                hpaValue += increase ? 1.0f : -1.0f;
                baroValue = hpaValue / 33.8639f;
            } else {
                baroValue += increase ? 0.01f : -0.01f;
            }
            datarefManager->set<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot", baroValue);
        }
    } else if (phase == xplm_CommandBegin && button->dataref == "map_plan_mode") {
        int currentStatus = datarefManager->getCached<int>("XCrafts/ERJ/MFD1/map_plan_status");
        int targetStatus = static_cast<int>(button->value);
        if (currentStatus != targetStatus) {
            datarefManager->executeCommand("XCrafts/ERJ/MFD1/butt_6_cmnd");
        }
    } else if (phase == xplm_CommandBegin && button->dataref == "map_plan_mode_copilot") {
        int currentStatus = datarefManager->getCached<int>("XCrafts/ERJ/MFD2/map_plan_status");
        int targetStatus = static_cast<int>(button->value);
        if (currentStatus != targetStatus) {
            datarefManager->executeCommand("XCrafts/ERJ/MFD2/butt_6_cmnd");
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

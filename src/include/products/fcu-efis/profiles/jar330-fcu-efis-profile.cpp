#include "jar330-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

JAR330FCUEfisProfile::JAR330FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    product->setAllLedsEnabled(false);

    // Monitor power and brightness
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 7) {
            return;
        }

        bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/battery_on");

        uint8_t target = hasPower ? brightness[6] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? 255 : 0);

        uint8_t screenBrightness = hasPower ? 255 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/battery_on");

    // TODO: Add JAR A330 autopilot LED monitoring if datarefs become available
    // Laminar A333 uses laminar/A333/annun/autopilot/* datarefs for AP/ATHR/LOC/APPR LED states
}

bool JAR330FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("jd/mcdu/big/line00col0");
}

const std::vector<std::string> &JAR330FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",

        // Autopilot speed (Airbus format)
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/airspeed_is_mach",

        // Heading
        "sim/cockpit/autopilot/heading_mag",

        // Altitude
        "sim/cockpit/autopilot/altitude",

        // Vertical speed
        "sim/cockpit/autopilot/vertical_velocity",

        // Barometer settings
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot",

        // JAR A330 specific barometer unit selection (if available)
        // "jd/barometer/capt_inHg_hPa_pos",
        // "jd/barometer/fo_inHg_hPa_pos",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &JAR330FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU main buttons
        {0, {"MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "sim/autopilot/NAV"}},
        {2, {"TRK", "sim/autopilot/trkfpa"}},
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {4, {"AP2", "sim/autopilot/servos2_toggle"}},
        {5, {"A/THR", "sim/autopilot/autothrottle_arm"}},
        {6, {"EXPED", "sim/autopilot/altitude_hold"}},
        {7, {"METRIC", "sim/autopilot/altitude_hold"}},
        {8, {"APPR", "sim/autopilot/approach"}},

        // Speed encoder
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {11, {"SPD PUSH", "sim/autopilot/autothrottle_toggle"}},
        {12, {"SPD PULL", "sim/autopilot/level_change"}},

        // Heading encoder
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/heading_sync"}},
        {16, {"HDG PULL", "sim/autopilot/heading"}},

        // Altitude encoder
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},
        {19, {"ALT PUSH", "sim/autopilot/altitude_sync"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_hold"}},

        // Vertical Speed encoder
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},
        {23, {"VS PUSH", "sim/autopilot/vertical_speed_sync"}},
        {24, {"VS PULL", "sim/autopilot/vertical_speed"}},

        {25, {"ALT 100", ""}},
        {26, {"ALT 1000", ""}},

        // EFIS Left (Captain) buttons (32-63)
        {32, {"L_FD", "sim/autopilot/fdir_toggle"}},
        {33, {"L_LS", "sim/instruments/EFIS_mode_up"}},
        {34, {"L_CSTR", "sim/instruments/EFIS_fix"}},
        {35, {"L_WPT", "sim/instruments/EFIS_fix"}},
        {36, {"L_VOR.D", "sim/instruments/EFIS_vor"}},
        {37, {"L_NDB", "sim/instruments/EFIS_ndb"}},
        {38, {"L_ARPT", "sim/instruments/EFIS_apt"}},
        {39, {"L_STD PUSH", "sim/instruments/barometer_std"}},
        {40, {"L_STD PULL", ""}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", ""}},
        {44, {"L_hPa", ""}},
        {45, {"L_MODE LS", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {48, {"L_MODE ARC", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {49, {"L_MODE PLAN", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {50, {"L_RANGE 10", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {56, {"L_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {57, {"L_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {58, {"L_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {59, {"L_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {60, {"L_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {61, {"L_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},

        // EFIS Right (FO) buttons (64-95)
        {64, {"R_FD", "sim/autopilot/fdir2_toggle"}},
        {65, {"R_LS", "sim/instruments/EFIS_copilot_mode_up"}},
        {66, {"R_CSTR", "sim/instruments/EFIS_copilot_fix"}},
        {67, {"R_WPT", "sim/instruments/EFIS_copilot_fix"}},
        {68, {"R_VOR.D", "sim/instruments/EFIS_copilot_vor"}},
        {69, {"R_NDB", "sim/instruments/EFIS_copilot_ndb"}},
        {70, {"R_ARPT", "sim/instruments/EFIS_copilot_apt"}},
        {71, {"R_STD PUSH", "sim/instruments/barometer_copilot_std"}},
        {72, {"R_STD PULL", ""}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", ""}},
        {76, {"R_hPa", ""}},
        {77, {"R_MODE LS", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {80, {"R_MODE ARC", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {81, {"R_MODE PLAN", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {88, {"R_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {89, {"R_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {90, {"R_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {91, {"R_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {92, {"R_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {93, {"R_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
    };

    return buttons;
}

void JAR330FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/battery_on");

    if (!hasPower) {
        data.speed = {"", {}};
        data.heading = {"", {}};
        data.altitude = {"", {}};
        data.verticalSpeed = {"", {}};
        data.efisRight.baro = "";
        data.efisLeft.baro = "";
        return;
    }

    data.headingHdg = true;
    data.headingLat = true;

    auto datarefManager = Dataref::getInstance();

    // Speed display
    bool isMach = datarefManager->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = datarefManager->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

    if (isMach) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << speed;
        std::string machStr = ss.str();
        if (machStr[0] == '0') {
            machStr = machStr.substr(1);
        }
        data.speed = {machStr, {}};
        data.spdMach = true;
    } else {
        std::ostringstream ss;
        ss << static_cast<int>(speed);
        data.speed = ss.str();
        data.spdMach = false;
    }

    // Heading display
    float heading = datarefManager->getCached<float>("sim/cockpit/autopilot/heading_mag");
    std::ostringstream headingSs;
    headingSs << std::setw(3) << std::setfill('0') << static_cast<int>(heading) % 360;
    data.heading = headingSs.str();

    // Altitude display
    float altitude = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
    std::ostringstream altSs;
    altSs << std::setw(5) << std::setfill('0') << static_cast<int>(altitude);
    data.altitude = altSs.str();

    // Vertical speed display
    float vs = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    std::ostringstream vsSs;
    int vsInt = static_cast<int>(vs);
    int absVs = std::abs(vsInt);
    vsSs << std::setw(4) << std::setfill('0') << absVs;
    data.verticalSpeed = vsSs.str();
    data.vsSign = (vs >= 0);
    data.vsMode = true;

    // Barometer displays (inHg)
    float baroPilot = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    float baroFO = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

    data.efisLeft.setBaro(baroPilot, true);
    data.efisRight.setBaro(baroFO, true);
}

void JAR330FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // Altitude 100/1000 step buttons
    if (button->name == "ALT 100" && phase == xplm_CommandBegin) {
        float current = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
        int rounded = (static_cast<int>(current) / 100) * 100;
        if (static_cast<int>(current) > rounded) {
            rounded += 100;
        }
        datarefManager->set<float>("sim/cockpit/autopilot/altitude", static_cast<float>(rounded));
        return;
    }

    if (button->name == "ALT 1000" && phase == xplm_CommandBegin) {
        float current = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
        int rounded = (static_cast<int>(current) / 1000) * 1000;
        if (static_cast<int>(current) > rounded) {
            rounded += 1000;
        }
        datarefManager->set<float>("sim/cockpit/autopilot/altitude", static_cast<float>(rounded));
        return;
    }

    if (button->dataref.empty()) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;
        const char *datarefName = isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
        float baroValue = datarefManager->getCached<float>(datarefName);
        bool increase = button->value > 0;
        float newBaro = baroValue + (increase ? 0.01f : -0.01f);
        datarefManager->set<float>(datarefName, newBaro);
    }
}

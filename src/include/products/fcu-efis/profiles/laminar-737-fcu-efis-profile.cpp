#include "laminar-737-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

Laminar737FCUEfisProfile::Laminar737FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // Monitor power and brightness
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/panel_brightness_ratio", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 4) {
            return;
        }

        bool avionicsOn = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");

        // Use appropriate brightness index for 737 instruments
        uint8_t target = avionicsOn ? brightness[0] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, avionicsOn ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, avionicsOn ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, avionicsOn ? 255 : 0);

        uint8_t screenBrightness = avionicsOn ? 255 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/panel_brightness_ratio");
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");

    // Monitor autopilot engagement (CMD A and CMD B) - use 737-specific status datarefs
    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/cmd_a_status", [product](int status) {
        // Status 2 = engaged and active
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, status == 1 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/cmd_b_status", [product](int status) {
        // Status 2 = engaged and active
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, status == 1 ? 255 : 0);
    }, this);

    // Monitor autothrottle arm - 0=disarmed, 1=armed
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/autothrottle_arm", [product](int armed) {
        // Light on when armed (1)
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, armed ? 255 : 0);
    }, this);

    // Monitor approach mode
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/approach_status", [product](int status) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, status > 0 ? 255 : 0);
    }, this);

    // Monitor localizer mode
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit/autopilot/autopilot_state", [product](int state) {
        // Check if LOC is active (bit 10)
        bool locActive = (state & (1 << 10)) != 0;

        product->setLedBrightness(FCUEfisLed::LOC_GREEN, locActive ? 255 : 0);
    }, this);

    // EFIS Right (FO) button LEDs
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/EFIS/EFIS_weather_on_copilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, on ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/EFIS/EFIS_tcas_on_copilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, on ? 255 : 0);
    }, this);

    // EFIS Left (Captain) button LEDs
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/EFIS/EFIS_weather_on_pilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, on ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/EFIS/EFIS_tcas_on_pilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, on ? 255 : 0);
    }, this);
}

bool Laminar737FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/autopilot/cmd_a_status") && Dataref::getInstance()->exists("zibomod/Aircraft_Path") == false;
}

const std::vector<std::string> &Laminar737FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/avionics_on",

        // Autopilot speed
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
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &Laminar737FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU main buttons
        {0, {"SPD/MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "sim/autopilot/NAV"}},
        {2, {"TRK", "sim/autopilot/trkfpa"}}, // TRK/FPA mode (may not work on default 737)
        // {3, {"AP1", "laminar/B738/autopilot/cmd_a_press"}},
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {4, {"AP2", "sim/autopilot/servos2_toggle"}},
        {5, {"A/THR", "custom"}},                       // Custom handler checks both arm and hard_off datarefs
        {6, {"EXPED", "sim/autopilot/vnav"}},           // VNAV (may not work on default 737)
        {7, {"METRIC", "sim/autopilot/altitude_hold"}}, // ALT HOLD as fallback
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

        {25, {"ALT 100", ""}},  // 100ft increment (handled in code)
        {26, {"ALT 1000", ""}}, // 1000ft increment (handled in code)

        // EFIS Left (Captain) buttons (32-63)
        {32, {"L_FD", "sim/autopilot/fdir_toggle"}},
        {33, {"L_LS", "sim/instruments/EFIS_mode_up"}},
        {34, {"L_CSTR", "sim/instruments/EFIS_fix"}},
        {35, {"L_WPT", "sim/instruments/EFIS_fix"}},
        {36, {"L_VOR.D", "sim/instruments/EFIS_vor"}},
        {37, {"L_NDB", "sim/instruments/EFIS_ndb"}},
        {38, {"L_ARPT", "sim/instruments/EFIS_apt"}},
        {39, {"L_STD PUSH", "sim/instruments/barometer_std"}},
        {40, {"L_STD PULL", ""}}, // No STD pull action for default 737
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", ""}}, // Always inHg on default 737
        {44, {"L_hPa", ""}},  // No hPa mode on default 737

        // ND mode selector (Left)
        {45, {"L_MODE LS", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {48, {"L_MODE ARC", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {49, {"L_MODE PLAN", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // Range selector (Left)
        {50, {"L_RANGE 10", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "sim/cockpit2/EFIS/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},

        // Nav source selector (Left)
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

        // ND mode selector (Right)
        {77, {"R_MODE LS", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {80, {"R_MODE ARC", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {81, {"R_MODE PLAN", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // Range selector (Right)
        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 5.0}},

        // Nav source selector (Right)
        {88, {"R_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {89, {"R_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {90, {"R_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {91, {"R_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {92, {"R_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {93, {"R_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
    };

    return buttons;
}

void Laminar737FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    bool avionicsOn = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on");

    if (!avionicsOn) {
        // Clear all displays when no avionics power
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

    // Speed display
    bool isMach = Dataref::getInstance()->getCached<int>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = Dataref::getInstance()->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

    if (isMach) {
        // Display as Mach number (e.g., "0.78" -> ".78")
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << speed;
        std::string machStr = ss.str();
        if (machStr[0] == '0') {
            machStr = machStr.substr(1); // Remove leading 0
        }
        data.speed = {machStr, {}};
        data.spdMach = true;

    } else {
        // Display as knots
        std::ostringstream ss;
        ss << static_cast<int>(speed);
        data.speed = ss.str();
        data.spdMach = false;
    }

    // Heading display
    float heading = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/heading_mag");
    std::ostringstream headingSs;
    headingSs << std::setw(3) << std::setfill('0') << static_cast<int>(heading);
    data.heading = headingSs.str();

    // Altitude display
    float altitude = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/altitude");
    std::ostringstream altSs;
    altSs << std::setw(5) << std::setfill('0') << static_cast<int>(altitude);
    data.altitude = altSs.str();

    // Vertical speed display
    float vs = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    std::ostringstream vsSs;
    int vsInt = static_cast<int>(vs);
    int absVs = std::abs(vsInt);
    vsSs << std::setw(4) << std::setfill('0') << absVs;
    data.verticalSpeed = vsSs.str();
    data.vsSign = (vs >= 0);
    data.vsMode = true;

    // Barometer displays (inHg)
    float baroPilot = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    float baroFO = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

    data.efisLeft.setBaro(baroPilot, true);
    data.efisRight.setBaro(baroFO, true);
}

void Laminar737FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // Special handling for A/THR button
    if (button->name == "A/THR" && phase == xplm_CommandBegin) {
        int armed = datarefManager->get<int>("sim/cockpit2/autopilot/autothrottle_arm");

        if (armed == 1) {
            // If armed, disable it by executing hard_off command
            datarefManager->executeCommand("sim/autopilot/autothrottle_hard_off");
        } else {
            // If not armed, arm it
            datarefManager->executeCommand("sim/autopilot/autothrottle_arm");
        }

        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        if (phase == xplm_CommandBegin) {
            int currentValue = datarefManager->get<int>(button->dataref.c_str());
            int newValue = currentValue == 0 ? 1 : 0;
            datarefManager->set<int>(button->dataref.c_str(), newValue);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        if (phase == xplm_CommandBegin) {
            datarefManager->set<float>(button->dataref.c_str(), button->value);
        }
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        if (phase == xplm_CommandBegin) {
            bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;
            const char *datarefName = isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
            float baroValue = datarefManager->getCached<float>(datarefName);
            bool increase = button->value > 0;
            float newBaro = baroValue + (increase ? 0.01f : -0.01f);
            datarefManager->set<float>(datarefName, newBaro);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

#include "zibo-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

ZiboFCUEfisProfile::ZiboFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
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

    // Monitor autopilot engagement (CMD A and CMD B) - use Zibo-specific status datarefs
    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/cmd_a_status", [product](int status) {
        // Status 1 = engaged and active
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, status == 1 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/cmd_b_status", [product](int status) {
        // Status 1 = engaged and active
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, status == 1 ? 255 : 0);
    }, this);

    // Monitor autothrottle arm - Zibo uses its own status dataref
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/autothrottle_status1", [product](float status) {
        // Status > 0 means armed
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, status > 0 ? 255 : 0);
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

    // EFIS Left (Captain) button LEDs
    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/EFIS/EFIS_weather_on_pilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, on ? 255 : 0);
    }, this);

    // Monitor flight director
    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/flight_director_pos", [product](int fdOn) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, fdOn > 0 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("laminar/B738/autopilot/flight_director_fo_pos", [product](int fdOn) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, fdOn > 0 ? 255 : 0);
    }, this);

    // Monitor barometer unit display state for proper display updates
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B738/EFIS_control/capt/baro_in_hpa", [](double hpaMode) {
        // Trigger display update when barometer mode changes
    }, this);

    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B738/EFIS_control/fo/baro_in_hpa", [](double hpaMode) {
        // Trigger display update when barometer mode changes
    }, this);
}

ZiboFCUEfisProfile::~ZiboFCUEfisProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool ZiboFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::vector<std::string> &ZiboFCUEfisProfile::displayDatarefs() const {
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

        // Barometer settings - use Zibo-specific datarefs
        "laminar/B738/EFIS/baro_sel_in_hg_pilot",
        "laminar/B738/EFIS/baro_sel_in_hg_copilot",

        // Barometer unit selection - triggers display updates
        "laminar/B738/EFIS_control/capt/baro_in_hpa",
        "laminar/B738/EFIS_control/fo/baro_in_hpa",

        // Vertical speed window visibility
        "laminar/B738/autopilot/vvi_dial_show",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &ZiboFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU main buttons
        {0, {"SPD/MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "laminar/B738/autopilot/vorloc_press"}},
        {2, {"TRK", "sim/autopilot/trkfpa"}},
        {3, {"AP1", "laminar/B738/autopilot/cmd_a_press"}},
        {4, {"AP2", "laminar/B738/autopilot/cmd_b_press"}},
        {5, {"A/THR", "laminar/B738/autopilot/autothrottle_arm_toggle"}},
        {6, {"EXPED", "laminar/B738/autopilot/vnav_press"}},
        {7, {"METRIC", "laminar/B738/autopilot/alt_hld_press"}},
        {8, {"APPR", "laminar/B738/autopilot/app_press"}},

        // Speed encoder
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {11, {"SPD PUSH", "sim/autopilot/autothrottle_toggle"}},
        {12, {"SPD PULL", "sim/autopilot/level_change"}},

        // Heading encoder
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "laminar/B738/autopilot/vnav_press", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},
        {16, {"HDG PULL", "laminar/B738/autopilot/hdg_sel_press", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},

        // Altitude encoder
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},
        {19, {"ALT PUSH", "laminar/B738/autopilot/lnav_press", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},
        {20, {"ALT PULL", "laminar/B738/autopilot/lvl_chg_press", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},

        // Vertical Speed encoder
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},
        {23, {"VS PUSH", "sim/autopilot/vertical_speed_sync"}},
        {24, {"VS PULL", "laminar/B738/autopilot/vs_press"}},

        {25, {"ALT 100", ""}},
        {26, {"ALT 1000", ""}},

        // EFIS Left (Captain) buttons (32-63)
        {32, {"L_FD", "laminar/B738/autopilot/flight_director_pos,laminar/B738/autopilot/flight_director_toggle,laminar/B738/autopilot/flight_director_toggle", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {33, {"L_LS", "sim/instruments/EFIS_mode_up"}},
        {34, {"L_CSTR", "sim/instruments/EFIS_fix"}},
        {35, {"L_WPT", "sim/instruments/EFIS_fix"}},
        {36, {"L_VOR.D", "sim/instruments/EFIS_vor"}},
        {37, {"L_NDB", "sim/instruments/EFIS_ndb"}},
        {38, {"L_ARPT", "sim/instruments/EFIS_apt"}},
        {39, {"L_STD PUSH", "laminar/B738/EFIS_control/capt/push_button/std_press"}},
        {40, {"L_STD PULL", "laminar/B738/EFIS_control/capt/push_button/std_pull"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "laminar/B738/EFIS_control/capt/baro_in_hpa_dn", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},
        {44, {"L_hPa", "laminar/B738/EFIS_control/capt/baro_in_hpa_up", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},

        // ND mode selector (Left)
        {45, {"L_MODE LS", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {47, {"L_MODE NAV", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {48, {"L_MODE ARC", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {49, {"L_MODE PLAN", "laminar/B738/EFIS_control/capt/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // Range selector (Left) - use Zibo's map_range dataref (0=5nm, 1=10nm, 2=20nm, 3=40nm, 4=80nm, 5=160nm, 6=320nm, 7=640nm)
        {50, {"L_RANGE 10", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {51, {"L_RANGE 20", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {52, {"L_RANGE 40", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {53, {"L_RANGE 80", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {54, {"L_RANGE 160", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {55, {"L_RANGE 320", "laminar/B738/EFIS/capt/map_range", FCUEfisDatarefType::SET_VALUE, 6.0}},

        // Nav source selector (Left)
        {56, {"L_1 ADF", "laminar/B738/EFIS_control/capt/vor1_off_pos,laminar/B738/EFIS_control/capt/vor1_off_dn,laminar/B738/EFIS_control/capt/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {57, {"L_1 OFF", "laminar/B738/EFIS_control/capt/vor1_off_pos,laminar/B738/EFIS_control/capt/vor1_off_dn,laminar/B738/EFIS_control/capt/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {58, {"L_1 VOR", "laminar/B738/EFIS_control/capt/vor1_off_pos,laminar/B738/EFIS_control/capt/vor1_off_dn,laminar/B738/EFIS_control/capt/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {59, {"L_2 ADF", "laminar/B738/EFIS_control/capt/vor2_off_pos,laminar/B738/EFIS_control/capt/vor2_off_dn,laminar/B738/EFIS_control/capt/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {60, {"L_2 OFF", "laminar/B738/EFIS_control/capt/vor2_off_pos,laminar/B738/EFIS_control/capt/vor2_off_dn,laminar/B738/EFIS_control/capt/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {61, {"L_2 VOR", "laminar/B738/EFIS_control/capt/vor2_off_pos,laminar/B738/EFIS_control/capt/vor2_off_dn,laminar/B738/EFIS_control/capt/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},

        // EFIS Right (FO) buttons (64-95)
        {64, {"R_FD", "laminar/B738/autopilot/flight_director_fo_pos,laminar/B738/autopilot/flight_director_fo_toggle,laminar/B738/autopilot/flight_director_fo_toggle", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {65, {"R_LS", "sim/instruments/EFIS_copilot_mode_up"}},
        {66, {"R_CSTR", "sim/instruments/EFIS_copilot_fix"}},
        {67, {"R_WPT", "sim/instruments/EFIS_copilot_fix"}},
        {68, {"R_VOR.D", "sim/instruments/EFIS_copilot_vor"}},
        {69, {"R_NDB", "sim/instruments/EFIS_copilot_ndb"}},
        {70, {"R_ARPT", "sim/instruments/EFIS_copilot_apt"}},
        {71, {"R_STD PUSH", "laminar/B738/EFIS_control/fo/push_button/std_press"}},
        {72, {"R_STD PULL", "laminar/B738/EFIS_control/fo/push_button/std_pull"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "laminar/B738/EFIS_control/fo/baro_in_hpa_dn", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},
        {76, {"R_hPa", "laminar/B738/EFIS_control/fo/baro_in_hpa_up", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},

        // ND mode selector (Right)
        {77, {"R_MODE LS", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {79, {"R_MODE NAV", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {80, {"R_MODE ARC", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {81, {"R_MODE PLAN", "laminar/B738/EFIS_control/fo/map_mode_pos", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // Range selector (Right) - use Zibo's map_range dataref (0=5nm, 1=10nm, 2=20nm, 3=40nm, 4=80nm, 5=160nm, 6=320nm, 7=640nm)
        {82, {"R_RANGE 10", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {83, {"R_RANGE 20", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {84, {"R_RANGE 40", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {85, {"R_RANGE 80", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {86, {"R_RANGE 160", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {87, {"R_RANGE 320", "laminar/B738/EFIS/fo/map_range", FCUEfisDatarefType::SET_VALUE, 6.0}},

        // Nav source selector (Right)
        {88, {"R_1 VOR", "laminar/B738/EFIS_control/fo/vor1_off_pos,laminar/B738/EFIS_control/fo/vor1_off_dn,laminar/B738/EFIS_control/fo/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {89, {"R_1 OFF", "laminar/B738/EFIS_control/fo/vor1_off_pos,laminar/B738/EFIS_control/fo/vor1_off_dn,laminar/B738/EFIS_control/fo/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {90, {"R_1 ADF", "laminar/B738/EFIS_control/fo/vor1_off_pos,laminar/B738/EFIS_control/fo/vor1_off_dn,laminar/B738/EFIS_control/fo/vor1_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {91, {"R_2 VOR", "laminar/B738/EFIS_control/fo/vor2_off_pos,laminar/B738/EFIS_control/fo/vor2_off_dn,laminar/B738/EFIS_control/fo/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {92, {"R_2 OFF", "laminar/B738/EFIS_control/fo/vor2_off_pos,laminar/B738/EFIS_control/fo/vor2_off_dn,laminar/B738/EFIS_control/fo/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {93, {"R_2 ADF", "laminar/B738/EFIS_control/fo/vor2_off_pos,laminar/B738/EFIS_control/fo/vor2_off_dn,laminar/B738/EFIS_control/fo/vor2_off_up", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
    };

    return buttons;
}

void ZiboFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
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

    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::LevelChangeHeader;
    data.headingHdg = true;
    data.headingLat = true;

    // Speed display
    bool isMach = Dataref::getInstance()->getCached<int>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = Dataref::getInstance()->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

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
    float heading = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/heading_mag");
    std::ostringstream headingSs;
    headingSs << std::setw(3) << std::setfill('0') << static_cast<int>(heading);
    data.heading = headingSs.str();

    // Altitude display
    float altitude = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/altitude");
    std::ostringstream altSs;
    altSs << std::setw(5) << std::setfill('0') << static_cast<int>(altitude);
    data.altitude = altSs.str();

    // Vertical speed display - only show when window is active
    bool vsWindowActive = Dataref::getInstance()->getCached<bool>("laminar/B738/autopilot/vvi_dial_show");
    if (vsWindowActive) {
        float vs = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
        std::ostringstream vsSs;
        int vsInt = static_cast<int>(vs);
        int absVs = std::abs(vsInt);
        vsSs << std::setw(4) << std::setfill('0') << absVs;
        data.verticalSpeed = vsSs.str();
        data.vsSign = (vs >= 0);
        data.vsMode = true;
    } else {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAValue;
    }

    // Barometer displays with Zibo unit selection
    float baroPilot = Dataref::getInstance()->getCached<float>("laminar/B738/EFIS/baro_sel_in_hg_pilot");
    float baroFO = Dataref::getInstance()->getCached<float>("laminar/B738/EFIS/baro_sel_in_hg_copilot");

    bool pilotInHpa = Dataref::getInstance()->getCached<bool>("laminar/B738/EFIS_control/capt/baro_in_hpa");
    bool foInHpa = Dataref::getInstance()->getCached<bool>("laminar/B738/EFIS_control/fo/baro_in_hpa");

    data.efisLeft.setBaro(baroPilot, !pilotInHpa);
    data.efisRight.setBaro(baroFO, !foInHpa);
}

void ZiboFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // Special handling for A/THR button - just execute the command
    if (button->name == "A/THR" && phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
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
            const char *datarefName = isCaptain ? "laminar/B738/EFIS/baro_sel_in_hg_pilot" : "laminar/B738/EFIS/baro_sel_in_hg_copilot";
            const char *unitRefName = isCaptain ? "laminar/B738/EFIS_control/capt/baro_in_hpa" : "laminar/B738/EFIS_control/fo/baro_in_hpa";

            float baroValue = datarefManager->get<float>(datarefName);
            bool inHpa = datarefManager->get<bool>(unitRefName);
            bool increase = button->value > 0;

            float increment = inHpa ? 0.02953f : 0.01f;
            float newBaro = baroValue + (increase ? increment : -increment);
            datarefManager->set<float>(datarefName, newBaro);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE_USING_COMMANDS) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> parts;
        while (std::getline(ss, item, ',')) {
            parts.push_back(item);
        }

        if (parts.size() >= 3) {
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
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

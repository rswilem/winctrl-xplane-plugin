#include "laminar-a333-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

LaminarA333FCUEfisProfile::LaminarA333FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit2/autopilot/autopilot_has_power");

        uint8_t target = hasPower ? brightness[14] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? 255 : 0);

        uint8_t screenBrightness = hasPower ? brightness[10] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/ap1_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/ap2_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/a_thr_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/loc_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/appr_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/autopilot/alt_mode", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    // Monitor EFIS Right (Captain) LED states
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/fo_flight_director_on", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/fo_ls_bars_on", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_LS_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_fo_cstr", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_fo_fix", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_fo_vor", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_fo_ndb", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_fo_arpt", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    // Monitor EFIS Left (First Officer) LED states
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/capt_flight_director_on", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/captain_ls_bars_on", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_LS_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_capt_cstr", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_capt_fix", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_capt_vor", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_capt_ndb", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/A333/annun/EFIS_capt_arpt", [product](float brightness) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, brightness > 0.9f ? 1 : 0);
    }, this);
}

LaminarA333FCUEfisProfile::~LaminarA333FCUEfisProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool LaminarA333FCUEfisProfile::IsEligible() {
    bool eligible = Dataref::getInstance()->exists("laminar/A333/ckpt_temp");
    return eligible;
}

const std::vector<std::string> &LaminarA333FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "laminar/A333/annun/autopilot/ap1_mode",
        "laminar/A333/annun/autopilot/ap2_mode",

        "sim/cockpit2/autopilot/vnav_speed_window_open",
        "laminar/A333/autopilot/hdg_window_open",
        "laminar/A333/autopilot/vvi_fpa_window_open",
        "sim/cockpit/autopilot/airspeed_is_mach",
        "sim/cockpit2/autopilot/vnav_speed_window_open",
        "sim/cockpit2/autopilot/trk_fpa",
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",
        "sim/cockpit2/autopilot/speed_status",
        "sim/cockpit2/autopilot/fms_vnav",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot",
        "laminar/A333/barometer/capt_inHg_hPa_pos",
        "laminar/A333/barometer/fo_inHg_hPa_pos",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &LaminarA333FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "sim/autopilot/NAV"}},
        {2, {"TRK", "sim/autopilot/trkfpa"}},
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {4, {"AP2", "sim/autopilot/servos2_toggle"}},
        {5, {"A/THR", "laminar/A333/autopilot/a_thr_toggle"}},
        {6, {"EXPED", "sim/autopilot/altitude_hold"}},
        {7, {"METRIC", "laminar/A333/autopilot/metric_alt_push"}},
        {8, {"APPR", "sim/autopilot/approach"}},
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {11, {"SPD PUSH", "laminar/A333/autopilot/speed_knob_push"}},
        {12, {"SPD PULL", "laminar/A333/autopilot/speed_knob_pull"}},
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "laminar/A333/autopilot/heading_knob_push"}},
        {16, {"HDG PULL", "laminar/A333/autopilot/heading_knob_pull"}},
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},
        {19, {"ALT PUSH", "laminar/A333/autopilot/altitude_knob_push"}},
        {20, {"ALT PULL", "laminar/A333/autopilot/altitude_knob_pull"}},
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},
        {23, {"VS PUSH", "laminar/A333/autopilot/vertical_knob_push"}},
        {24, {"VS PULL", "laminar/A333/autopilot/vertical_knob_pull"}},
        {25, {"ALT 100", "laminar/A333/autopilot/alt_step_left"}},
        {26, {"ALT 1000", "laminar/A333/autopilot/alt_step_right"}},

        // Brightness control buttons
        // These are handled internally via brightness callback system
        // {27, "BRIGHT", "AirbusFBW/SupplLightLevelRehostats[0]"},  // Panel brightness
        // {27, "BRIGHT_LCD", "AirbusFBW/SupplLightLevelRehostats[1]"},  // LCD brightness

        // LED control buttons for monitoring only (no direct button presses)
        // {28, "APPR_LED", "AirbusFBW/APPRilluminated"},  // Monitored via LED callback
        // {29, "ATHR_LED", "AirbusFBW/ATHRmode"},  // Monitored via LED callback
        // {30, "LOC_LED", "AirbusFBW/LOCilluminated"},  // Monitored via LED callback
        // Button 31 reserved

        // EFIS Left (Pilot) buttons (64-95)
        {32, {"L_FD", "sim/autopilot/fdir_command_bars_toggle"}},
        {33, {"L_LS", "laminar/A333/buttons/capt_ils_bars_push"}},
        {34, {"L_CSTR", "laminar/A333/buttons/capt_EFIS_CSTR"}},
        {35, {"L_WPT", "sim/instruments/EFIS_fix"}},
        {36, {"L_VOR.D", "sim/instruments/EFIS_vor"}},
        {37, {"L_NDB", "sim/instruments/EFIS_ndb"}},
        {38, {"L_ARPT", "sim/instruments/EFIS_apt"}},
        {39, {"L_STD PUSH", "laminar/A333/push/baro/capt_std"}},
        {40, {"L_STD PULL", "laminar/A333/pull/baro/capt_std"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "laminar/A333/knob/baro/capt_inHg"}},
        {44, {"L_hPa", "laminar/A333/knob/baro/capt_hPa"}}, // Set to 1 for hPa
        {45, {"L_MODE LS", "laminar/A333/knobs/EFIS_mode_pos_capt,laminar/A333/knobs/capt_EFIS_knob_left,laminar/A333/knobs/capt_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {46, {"L_MODE VOR", "laminar/A333/knobs/EFIS_mode_pos_capt,laminar/A333/knobs/capt_EFIS_knob_left,laminar/A333/knobs/capt_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {47, {"L_MODE NAV", "laminar/A333/knobs/EFIS_mode_pos_capt,laminar/A333/knobs/capt_EFIS_knob_left,laminar/A333/knobs/capt_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {48, {"L_MODE ARC", "laminar/A333/knobs/EFIS_mode_pos_capt,laminar/A333/knobs/capt_EFIS_knob_left,laminar/A333/knobs/capt_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
        {49, {"L_MODE PLAN", "laminar/A333/knobs/EFIS_mode_pos_capt,laminar/A333/knobs/capt_EFIS_knob_left,laminar/A333/knobs/capt_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
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
        // Buttons 62-63 reserved

        {64, {"R_FD", "sim/autopilot/fdir2_command_bars_toggle"}},
        {65, {"R_LS", "laminar/A333/buttons/fo_ils_bars_push"}},
        {66, {"R_CSTR", "laminar/A333/buttons/fo_EFIS_CSTR"}},
        {67, {"R_WPT", "sim/instruments/EFIS_copilot_fix"}},
        {68, {"R_VOR.D", "sim/instruments/EFIS_copilot_vor"}},
        {69, {"R_NDB", "sim/instruments/EFIS_copilot_ndb"}},
        {70, {"R_ARPT", "sim/instruments/EFIS_copilot_apt"}},
        {71, {"R_STD PUSH", "laminar/A333/push/baro/fo_std"}},
        {72, {"R_STD PULL", "laminar/A333/pull/baro/fo_std"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "laminar/A333/knob/baro/fo_inHg"}},
        {76, {"R_hPa", "laminar/A333/knob/baro/fo_hPa"}},
        {77, {"R_MODE LS", "laminar/A333/knobs/EFIS_mode_pos_fo,laminar/A333/knobs/fo_EFIS_knob_left,laminar/A333/knobs/fo_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {78, {"R_MODE VOR", "laminar/A333/knobs/EFIS_mode_pos_fo,laminar/A333/knobs/fo_EFIS_knob_left,laminar/A333/knobs/fo_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {79, {"R_MODE NAV", "laminar/A333/knobs/EFIS_mode_pos_fo,laminar/A333/knobs/fo_EFIS_knob_left,laminar/A333/knobs/fo_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {80, {"R_MODE ARC", "laminar/A333/knobs/EFIS_mode_pos_fo,laminar/A333/knobs/fo_EFIS_knob_left,laminar/A333/knobs/fo_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
        {81, {"R_MODE PLAN", "laminar/A333/knobs/EFIS_mode_pos_fo,laminar/A333/knobs/fo_EFIS_knob_left,laminar/A333/knobs/fo_EFIS_knob_right", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range_copilot", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {88, {"R_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {89, {"R_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {90, {"R_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {91, {"R_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {92, {"R_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {93, {"R_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},

        // Buttons 94-95 reserved
    };
    return buttons;
}

void LaminarA333FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.spdManaged = datarefManager->getCached<bool>("sim/cockpit2/autopilot/vnav_speed_window_open") == false;
    data.hdgManaged = datarefManager->getCached<bool>("laminar/A333/autopilot/hdg_window_open") == false;
    data.altManaged = datarefManager->getCached<bool>("sim/cockpit2/autopilot/fms_vnav");

    data.spdMach = datarefManager->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = datarefManager->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

    if (speed > 0 && datarefManager->getCached<bool>("sim/cockpit2/autopilot/vnav_speed_window_open")) {
        std::stringstream ss;
        if (data.spdMach) {
            // In Mach mode, format as 0.XX -> "0XX" (e.g., 0.40 -> "040", 0.82 -> "082")
            int machHundredths = static_cast<int>(std::round(speed * 100));
            ss << std::setfill('0') << std::setw(3) << machHundredths;
        } else {
            // In speed mode, format as regular integer
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(speed);
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }

    // Format FCU heading display - using sim/cockpit/autopilot/heading_mag (float)
    float heading = datarefManager->getCached<float>("sim/cockpit/autopilot/heading_mag");
    if (heading >= 0 && data.hdgManaged == false) {
        // Convert 360 to 0 for display
        int hdgDisplay = static_cast<int>(heading) % 360;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }

    // Format FCU altitude display - using sim/cockpit/autopilot/altitude (float)
    float altitude = datarefManager->getCached<float>("sim/cockpit/autopilot/altitude");
    if (altitude >= 0) {
        int altInt = static_cast<int>(altitude);
        std::stringstream ss;
        // Always show full altitude value
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    // Format vertical speed display - using sim/cockpit/autopilot/vertical_velocity (float)
    float vs = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    bool vsDashed = datarefManager->getCached<int>("laminar/A333/autopilot/vvi_fpa_window_open") == false;

    // HDG/TRK mode
    data.headingTrk = datarefManager->getCached<bool>("sim/cockpit2/autopilot/trk_fpa");
    data.headingHdg = !data.headingTrk;

    data.vsMode = !data.headingTrk; // VS mode when HDG mode
    data.fpaMode = data.headingTrk; // FPA mode when TRK mode

    if (vsDashed) {
        // When dashed, show 5 dashes with minus sign
        data.verticalSpeed = "-----";
        data.vsSign = false;          // Show minus sign for dashes
        data.fpaComma = data.fpaMode; // Show decimal point only in FPA mode
    } else if (data.fpaMode) {
        // In FPA mode (TRK mode), display format X.Y
        // Dataref value 600 corresponds to indication +0.6
        // Convert dataref value to FPA display: divide by 1000
        float fpa = vs / 1000.0f; // 600 -> 0.6, -100 -> -0.1

        float absFpa = std::abs(fpa);

        // Format FPA with only significant digits
        // 0.0 should be "00  " to display as "0.0"
        // 0.6 should be "06  " to display as "0.6"
        // 1.2 should be "12  " to display as "1.2"
        // 2.5 should be "25  " to display as "2.5"

        int fpaTenths = static_cast<int>(std::round(absFpa * 10)); // 0.0->0, 0.6->6, 1.2->12, 2.5->25

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << fpaTenths << "  "; // 2 digits + 2 spaces

        data.verticalSpeed = ss.str();

        data.fpaComma = true;     // Enable decimal point display
        data.vsSign = (fpa >= 0); // Control sign display for FPA
    } else {
        // Normal VS mode: Format with proper padding to 4 digits (no sign in string)
        std::stringstream ss;
        int vsInt = static_cast<int>(std::round(vs));
        int absVs = std::abs(vsInt);

        // If VS is a multiple of 100, show last two digits as "##"
        if (absVs % 100 == 0) {
            ss << std::setfill('0') << std::setw(2) << (absVs / 100) << "##";
        } else {
            // Show full value for non-multiples of 100
            ss << std::setfill('0') << std::setw(4) << absVs;
        }
        data.verticalSpeed = ss.str();

        data.vsSign = (vs >= 0); // Control sign display for VS
        data.fpaComma = false;   // No decimal point in VS mode
    }

    // Set VS/FPA indication flags based on current mode
    data.vsIndication = data.vsMode;   // fvs flag - show VS indication when in VS mode
    data.fpaIndication = data.fpaMode; // ffpa2 flag - show FPA indication when in FPA mode

    // VS vertical line
    // Only show vertical line in VS mode when not dashed
    data.vsVerticalLine = data.vsMode && (data.verticalSpeed != "-----");

    // LAT mode - Typically always on for Airbus
    data.headingLat = true;

    for (int i = 0; i < 2; i++) {
        bool isCaptain = i == 0;

        bool isStd = datarefManager->getCached<bool>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot");
        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "laminar/A333/barometer/capt_inHg_hPa_pos" : "laminar/A333/barometer/fo_inHg_hPa_pos");
        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .baro = "",
            .unitIsInHg = false,
            .isStd = isStd,
        };

        if (!isStd && baroValue > 0) {
            value.setBaro(baroValue, !isBaroHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void LaminarA333FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;
        bool isStd = datarefManager->getCached<bool>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_is_std_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_is_std_copilot");
        if (isStd) {
            return;
        }

        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "laminar/A333/barometer/capt_inHg_hPa_pos" : "laminar/A333/barometer/fo_inHg_hPa_pos");
        const char *datarefName = isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        bool increase = button->value > 0;

        if (isBaroHpa) {
            float hpaValue = baroValue * 33.8639f;
            hpaValue += increase ? 1.0f : -1.0f;
            baroValue = hpaValue / 33.8639f;
        } else {
            baroValue += increase ? 0.01f : -0.01f;
        }

        datarefManager->set<float>(datarefName, baroValue);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE_USING_COMMANDS) {
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
        return;
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::SET_VALUE || button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE)) {
        bool wantsToggle = button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE;

        if (wantsToggle) {
            int currentValue = datarefManager->get<int>(button->dataref.c_str());
            int newValue = currentValue ? 0 : 1;
            datarefManager->set<int>(button->dataref.c_str(), newValue);
        } else {
            datarefManager->set<float>(button->dataref.c_str(), button->value);
        }

        return;
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

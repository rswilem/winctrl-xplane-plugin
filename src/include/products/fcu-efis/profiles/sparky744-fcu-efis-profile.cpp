#include "sparky744-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

SparkyB744FCUEfisProfile::SparkyB744FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool powered) {
        uint8_t backlight = powered ? 200 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, backlight);

        uint8_t screenBrightness = powered ? 200 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, powered ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, powered ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, powered ? 255 : 0);

        product->forceStateSync();
    }, this);

    // MCP engagement LEDs
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/cmd_L_mode/status", [product](double engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/cmd_R_mode/status", [product](double engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autothrottle/armed", [product](double armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, armed > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/nav_status", [product](double status) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, status > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/glideslope_status", [product](double status) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, status > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/lnav_state", [product](double state) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, state > 0.5 ? 1 : 0);
    }, this);

    // EFIS Left (captain) LEDs
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/AFDS/status_annun_pilot", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/capt/wpt", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/capt/vor_ndb", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/capt/apt", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, on > 0.5 ? 1 : 0);
    }, this);

    // EFIS Right (FO) LEDs
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/AFDS/status_annun_copilot", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/fo/wpt", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/fo/vor_ndb", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, on > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/nd/data/fo/apt", [product](double on) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, on > 0.5 ? 1 : 0);
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
}

bool SparkyB744FCUEfisProfile::IsEligible() {
    auto dr = Dataref::getInstance();
    return dr->exists("laminar/B747/fms1/Line01_L") && !dr->exists("FPS/748/simtime") && !dr->exists("SSG/748/simtime");
}

const std::vector<std::string> &SparkyB744FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/avionics_on",
        "laminar/B747/autopilot/ias_dial_value",
        "laminar/B747/autopilot/ias_mach/window_open",
        "laminar/B747/autopilot/heading/degrees",
        "laminar/B747/autopilot/heading/altitude_dial_ft",
        "laminar/B747/cockpit2/autopilot/vvi_dial_fpm",
        "laminar/B747/autopilot/vert_spd/window_open",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "laminar/B747/efis/baro/capt/set_dial_pos",
        "laminar/B747/efis/baro_std/capt/switch_pos",
        "laminar/B747/efis/baro_ref/capt/sel_dial_pos",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "laminar/B747/efis/baro/fo/set_dial_pos",
        "laminar/B747/efis/baro_std/fo/switch_pos",
        "laminar/B747/efis/baro_ref/fo/sel_dial_pos",
        "laminar/B747/nd/mode/capt/sel_dial_pos",
        "laminar/B747/nd/range/capt/sel_dial_pos",
        "laminar/B747/nd/mode/fo/sel_dial_pos",
        "laminar/B747/nd/range/fo/sel_dial_pos"};
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &SparkyB744FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {

        // MCP --------------------------------------------------------------------------
        // Buttons
        {0, {"SPD", "laminar/B747/autopilot/button_switch/speed_mode"}},
        {1, {"LOC", "laminar/B747/autopilot/button_switch/loc_mode"}},
        {3, {"AP1", "laminar/B747/autopilot/button_switch/cmd_L"}},
        {4, {"AP2", "laminar/B747/autopilot/button_switch/cmd_R"}},
        {5, {"A/THR", "laminar/B747/toggle_switch/autothrottle", FCUEfisDatarefType::EXECUTE_CMD_ONCE}},
        {6, {"LNAV", "laminar/B747/autopilot/button_switch/LNAV"}},
        {7, {"V/S", "laminar/B747/autopilot/button_switch/vs_mode"}},
        {8, {"APP", "laminar/B747/autopilot/approach"}},

        // Rotary encoders - Speed
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {11, {"SPD IAS/MACH", "laminar/B747/autopilot/ias_mach/sel_button"}},

        // Rotary encoders - Heading
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG SEL", "laminar/B747/autopilot/button_switch/heading_select"}},
        {16, {"HDG HOLD", "laminar/B747/autopilot/button_switch/alt_hold_mode"}},

        // Rotary encoders - Altitude
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},
        {20, {"ALT HOLD", "laminar/B747/autopilot/button_switch/alt_hold_mode"}},

        // Rotary encoders - Vertical Speed
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},
        {23, {"V/S", "laminar/B747/autopilot/button_switch/vs_mode"}},

        // EFIS CAPT --------------------------------------------------------------------
        {32, {"L_FD", "laminar/B747/toggle_switch/flight_dir_L"}},
        {33, {"AP DISC", "laminar/B747/autopilot/slide_switch/disengage_bar"}},

        // ND overlay options
        {35, {"L_WPT", "laminar/B747/nd/wpt/capt/switch"}},
        {36, {"L_STA", "laminar/B747/nd/sta/capt/switch"}},
        {38, {"L_ARPT", "laminar/B747/nd/arpt/capt/switch"}},

        // BARO
        {39, {"L_BARO PUSH", "laminar/B747/efis/baro_std/capt/switch_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {40, {"L_BARO PULL", "laminar/B747/efis/baro_std/capt/switch_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {41, {"L_BARO DEC", "laminar/B747/efis/baro_set/capt/sel_dial_dn"}},
        {42, {"L_BARO INC", "laminar/B747/efis/baro_set/capt/sel_dial_up"}},
        {43, {"L_inHg", "laminar/B747/efis/baro_ref/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {44, {"L_hPa", "laminar/B747/efis/baro_ref/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},

        // ND Mode selector
        {45, {"L_MODE APP", "laminar/B747/nd/mode/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {46, {"L_MODE VOR", "laminar/B747/nd/mode/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {48, {"L_MODE MAP", "laminar/B747/nd/mode/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 2}},
        {49, {"L_MODE PLAN", "laminar/B747/nd/mode/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 3}},

        // ND Range selector
        {50, {"L_RANGE 10", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {51, {"L_RANGE 20", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 2}},
        {52, {"L_RANGE 40", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 3}},
        {53, {"L_RANGE 80", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 4}},
        {54, {"L_RANGE 160", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 5}},
        {55, {"L_RANGE 320", "laminar/B747/nd/range/capt/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 6}},

        // EFIS FO ----------------------------------------------------------------------
        {64, {"R_FD", "laminar/B747/toggle_switch/flight_dir_R"}},
        {65, {"AP DISC", "laminar/B747/autopilot/slide_switch/disengage_bar"}},

        // ND overlay options
        {67, {"R_WPT", "laminar/B747/nd/wpt/fo/switch"}},
        {68, {"R_STA", "laminar/B747/nd/sta/fo/switch"}},
        {70, {"R_ARPT", "laminar/B747/nd/arpt/fo/switch"}},

        // BARO
        {71, {"R_BARO PUSH", "laminar/B747/efis/baro_std/fo/switch_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {72, {"R_BARO PULL", "laminar/B747/efis/baro_std/fo/switch_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {73, {"R_BARO DEC", "laminar/B747/efis/baro_set/fo/sel_dial_dn"}},
        {74, {"R_BARO INC", "laminar/B747/efis/baro_set/fo/sel_dial_up"}},
        {75, {"R_inHg", "laminar/B747/efis/baro_ref/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {76, {"R_hPa", "laminar/B747/efis/baro_ref/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},

        // ND Mode selector
        {77, {"R_MODE APP", "laminar/B747/nd/mode/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 0}},
        {78, {"R_MODE VOR", "laminar/B747/nd/mode/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {80, {"R_MODE MAP", "laminar/B747/nd/mode/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 2}},
        {81, {"R_MODE PLAN", "laminar/B747/nd/mode/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 3}},

        // ND Range selector
        {82, {"R_RANGE 10", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 1}},
        {83, {"R_RANGE 20", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 2}},
        {84, {"R_RANGE 40", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 3}},
        {85, {"R_RANGE 80", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 4}},
        {86, {"R_RANGE 160", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 5}},
        {87, {"R_RANGE 320", "laminar/B747/nd/range/fo/sel_dial_pos", FCUEfisDatarefType::SET_VALUE, 6}},
    };
    return buttons;
}

void SparkyB744FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dm = Dataref::getInstance();

    data.displayEnabled = dm->getCached<bool>("sim/cockpit/electrical/avionics_on");
    data.displayTest = false;

    // Speed
    double speedTarget = dm->getCached<double>("laminar/B747/autopilot/ias_dial_value");
    bool windowOpen = dm->getCached<double>("laminar/B747/autopilot/ias_mach/window_open") > 0.5;
    bool isMach = false;
    data.spdMach = isMach;

    if (!windowOpen || speedTarget < 0) {
        data.speed = "---";
    } else {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << static_cast<int>(std::round(speedTarget));
        data.speed = ss.str();
    }
    data.spdManaged = false;

    // Heading
    data.headingHdg = true;
    data.headingTrk = false;
    data.headingLat = true;

    double heading = dm->getCached<double>("laminar/B747/autopilot/heading/degrees");
    if (heading >= 0) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << (static_cast<int>(heading) % 360);
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }
    data.hdgManaged = false;

    // Altitude
    double altitude = dm->getCached<double>("laminar/B747/autopilot/heading/altitude_dial_ft");
    if (altitude > 0) {
        int altInt = static_cast<int>(std::round(altitude / 100.0) * 100);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }
    data.altManaged = false;

    // Vertical speed
    bool vsWindowOpen = dm->getCached<double>("laminar/B747/autopilot/vert_spd/window_open") > 0.5;
    double vs = dm->getCached<double>("laminar/B747/cockpit2/autopilot/vvi_dial_fpm");

    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    data.fpaIndication = false;
    data.fpaComma = false;
    data.vsVerticalLine = vsWindowOpen;

    if (!vsWindowOpen) {
        data.verticalSpeed = "----";
        data.vsSign = true;
    } else {
        std::stringstream ss;
        int absVs = std::abs(static_cast<int>(std::round(vs)));
        ss << std::setfill('0') << std::setw(4) << absVs;
        data.verticalSpeed = ss.str();
        data.vsSign = (vs >= 0);
    }

    // EFIS baro — preselect is in inHg, convert to hPa if unit selector says so
    for (int i = 0; i < 2; i++) {
        bool isCaptain = (i == 0);
        const char *baroValueRef = isCaptain
                                     ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot"
                                     : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
        const char *baroStdRef = isCaptain ? "laminar/B747/efis/baro_std/capt/switch_pos" : "laminar/B747/efis/baro_std/fo/switch_pos";
        const char *baroTypeRef = isCaptain ? "laminar/B747/efis/baro_ref/capt/sel_dial_pos" : "laminar/B747/efis/baro_ref/fo/sel_dial_pos";

        float baroInhg = dm->getCached<float>(baroValueRef);
        bool isStd = dm->getCached<double>(baroStdRef) > 0.5;
        bool baroIsHpa = dm->getCached<double>(baroTypeRef) > 0.5;
        float baroValue = baroIsHpa ? baroInhg * 33.8639f : baroInhg;

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = !baroIsHpa,
            .isStd = isStd,
        };

        if (!isStd && baroValue > 0) {
            value.setBaro(baroValue, !baroIsHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void SparkyB744FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        float current = dm->get<float>(button->dataref.c_str());
        dm->set<float>(button->dataref.c_str(), current > 0.5f ? 0.0f : 1.0f);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str(), phase);
    } else {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

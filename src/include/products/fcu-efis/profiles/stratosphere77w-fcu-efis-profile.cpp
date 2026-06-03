#include "stratosphere77w-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

Strato77WFCUEfisProfile::Strato77WFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [product](bool powered) {
        uint8_t backlight = powered ? 200 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, powered ? 200 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, powered ? 200 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, powered ? 200 : 0);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, powered ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, powered ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, powered ? 255 : 0);
        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/ap_on", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/at_arm", [product](int armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, armed ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/flt_dir_pilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, on ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/flt_dir_copilot", [product](int on) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, on ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/cockpit/lights/caut_cap", [product](int on) {
        bool warn = Dataref::getInstance()->getCached<bool>("Strato/777/cockpit/lights/warn_cap");
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, on || warn ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, on || warn ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, on || warn ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/cockpit/lights/caut_fo", [product](int on) {
        bool warn = Dataref::getInstance()->getCached<bool>("Strato/777/cockpit/lights/warn_fo");
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, on || warn ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, on || warn ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, on || warn ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/cockpit/lights/warn_cap", [product](int on) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Strato/777/cockpit/lights/caut_cap");
    });

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/cockpit/lights/warn_fo", [product](int on) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Strato/777/cockpit/lights/caut_fo");
    });

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/autopilot_has_power");
}

Strato77WFCUEfisProfile::~Strato77WFCUEfisProfile() {
    Dataref::getInstance()->unbind("sim/cockpit2/autopilot/autopilot_has_power");
    Dataref::getInstance()->unbind("Strato/777/mcp/ap_on");
    Dataref::getInstance()->unbind("Strato/777/mcp/at_arm");
    Dataref::getInstance()->unbind("Strato/777/mcp/flt_dir_pilot");
    Dataref::getInstance()->unbind("Strato/777/mcp/flt_dir_copilot");
    Dataref::getInstance()->unbind("Strato/777/cockpit/lights/caut_cap");
    Dataref::getInstance()->unbind("Strato/777/cockpit/lights/caut_fo");
    Dataref::getInstance()->unbind("Strato/777/cockpit/lights/warn_cap");
    Dataref::getInstance()->unbind("Strato/777/cockpit/lights/warn_fo");
}

bool Strato77WFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("Strato/777/mcp/ap_on") &&
           Dataref::getInstance()->exists("Strato/B777/fms1/Line01_L");
}

const std::vector<std::string> &Strato77WFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/autopilot/autopilot_has_power",

        // MCP display values
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/airspeed_is_mach",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",

        // MCP mode states
        "Strato/777/mcp/hdg_to_trk",
        "Strato/777/mcp/vs_fpa",
        "Strato/777/mcp/vshold",

        // EFIS baro
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "Strato/777/baro_mode",
        "Strato/777/displays/alt_std",

        // ND mode and range
        "Strato/777/fltInst/nd_mode_selector",
        "Strato/777/map_zoom_knob",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &Strato77WFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {

        // MCP top row buttons
        {0, {"IAS/MACH", "Strato/B777/button_switch/mcp/spd_mode"}},
        {1, {"LOC", "Strato/B777/button_switch/mcp/ap/loc"}},
        {2, {"HDG/TRK", "Strato/B777/button_switch/mcp/hdg_mode"}},
        {3, {"AP1", "Strato/B777/button_switch/mcp/ap/engage_1"}},
        {4, {"AP2", "Strato/B777/button_switch/mcp/ap/engage_2"}},
        {5, {"A/THR", "Strato/B777/button_switch/mcp/at_arm"}},
        {6, {"VNAV", "Strato/B777/button_switch/mcp/ap/vnav"}},
        {7, {"VS/FPA", "Strato/B777/button_switch/mcp/vs_mode"}},
        {8, {"APP", "Strato/B777/button_switch/mcp/ap/app"}},

        // Speed rotary
        {9, {"SPD DEC", "Strato/777/spd_dn"}},
        {10, {"SPD INC", "Strato/777/spd_up"}},
        {11, {"SPD PUSH", "Strato/B777/button_switch/mcp/autothrottle/spd"}},
        {12, {"SPD PULL", "Strato/B777/button_switch/mcp/ap/flch"}},

        // Heading rotary
        {13, {"HDG DEC", "Strato/777/hdg_dn"}},
        {14, {"HDG INC", "Strato/777/hdg_up"}},
        {15, {"HDG PUSH", "Strato/B777/button_switch/mcp/ap/lnav"}},
        {16, {"HDG PULL", "Strato/B777/button_switch/mcp/ap/hdgSel"}},

        // Altitude rotary
        {17, {"ALT DEC", "Strato/777/autopilot/alt_dn"}},
        {18, {"ALT INC", "Strato/777/autopilot/alt_up"}},
        {19, {"ALT PUSH", "Strato/B777/button_switch/mcp/ap/vnav"}},
        {20, {"ALT PULL", "Strato/B777/button_switch/mcp/ap/flch"}},

        // VS rotary
        {21, {"VS DEC", "Strato/777/autopilot/vs_dn"}},
        {22, {"VS INC", "Strato/777/autopilot/vs_up"}},
        {23, {"VS PUSH", "Strato/B777/button_switch/mcp/ap/altHold"}},
        {24, {"VS PULL", "Strato/B777/button_switch/mcp/ap/vs"}},

        // EFIS Captain (left) ---------------------------------------------------------
        {32, {"L_FD", "Strato/B777/button_switch/mcp/fd_capt"}},
        {33, {"L_AP DISC", "Strato/B777/button_switch/mcp/ap/disengage"}},

        {35, {"L_WPT", "Strato/777/button_switch/efis/wpt"}},
        {36, {"L_STA", "Strato/777/button_switch/efis/sta"}},
        {38, {"L_ARPT", "Strato/777/button_switch/efis/arpt"}},

        // Baro
        {39, {"L_STD PUSH", "Strato/777/altm_baro_rst_capt"}},
        {40, {"L_STD PULL", "Strato/777/altm_baro_rst_capt"}},
        {41, {"L_PRESS DEC", "Strato/777/altm_baro_dn_capt"}},
        {42, {"L_PRESS INC", "Strato/777/altm_baro_up_capt"}},
        {43, {"L_inHg", "array_set:Strato/777/baro_mode:0", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "array_set:Strato/777/baro_mode:0", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // ND Mode selector
        {45, {"L_MODE APP", "array_set:Strato/777/fltInst/nd_mode_selector:0", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "array_set:Strato/777/fltInst/nd_mode_selector:0", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", ""}},
        {48, {"L_MODE MAP", "array_set:Strato/777/fltInst/nd_mode_selector:0", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {49, {"L_MODE PLAN", "array_set:Strato/777/fltInst/nd_mode_selector:0", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // ND Range selector
        {50, {"L_RANGE 10", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "array_set:Strato/777/map_zoom_knob:0", FCUEfisDatarefType::SET_VALUE, 5.0}},

        // EFIS First Officer (right) --------------------------------------------------
        {64, {"R_FD", "Strato/B777/button_switch/mcp/fd_fo"}},
        {65, {"R_AP DISC", "Strato/B777/button_switch/mcp/ap/disengage"}},

        {67, {"R_WPT", "Strato/777/button_switch/efis/wpt_fo"}},
        {68, {"R_STA", "Strato/777/button_switch/efis/sta_fo"}},
        {70, {"R_ARPT", "Strato/777/button_switch/efis/arpt_fo"}},

        // Baro
        {71, {"R_STD PUSH", "Strato/777/altm_baro_rst_fo"}},
        {72, {"R_STD PULL", "Strato/777/altm_baro_rst_fo"}},
        {73, {"R_PRESS DEC", "Strato/777/altm_baro_dn_fo"}},
        {74, {"R_PRESS INC", "Strato/777/altm_baro_up_fo"}},
        {75, {"R_inHg", "array_set:Strato/777/baro_mode:1", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "array_set:Strato/777/baro_mode:1", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // ND Mode selector
        {77, {"R_MODE APP", "array_set:Strato/777/fltInst/nd_mode_selector:1", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "array_set:Strato/777/fltInst/nd_mode_selector:1", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", ""}},
        {80, {"R_MODE MAP", "array_set:Strato/777/fltInst/nd_mode_selector:1", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {81, {"R_MODE PLAN", "array_set:Strato/777/fltInst/nd_mode_selector:1", FCUEfisDatarefType::SET_VALUE, 3.0}},

        // ND Range selector
        {82, {"R_RANGE 10", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "array_set:Strato/777/map_zoom_knob:1", FCUEfisDatarefType::SET_VALUE, 5.0}},
    };
    return buttons;
}

void Strato77WFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dm = Dataref::getInstance();

    data.displayEnabled = dm->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayTest = false;
    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::LevelChangeHeader;

    // Speed
    bool isMach = dm->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = dm->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");
    data.spdMach = isMach;

    if (speed > 0) {
        std::stringstream ss;
        if (isMach) {
            int machHundredths = static_cast<int>(std::round(speed * 100));
            ss << std::setfill('0') << std::setw(3) << machHundredths;
        } else {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(std::round(speed));
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }
    data.spdManaged = false;

    // Heading
    bool isHdg = !dm->getCached<bool>("Strato/777/mcp/hdg_to_trk");
    data.headingHdg = isHdg;
    data.headingTrk = !isHdg;
    data.headingLat = false;

    float heading = dm->getCached<float>("sim/cockpit/autopilot/heading_mag");
    if (heading >= 0) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << (static_cast<int>(std::round(heading)) % 360);
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }
    data.hdgManaged = false;

    // Altitude
    float altitude = dm->getCached<float>("sim/cockpit/autopilot/altitude");
    if (altitude >= 0 && altitude < 100000) {
        int altInt = static_cast<int>(std::round(altitude / 100.0) * 100);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }
    data.altManaged = false;

    // Vertical speed
    bool isVsMode = !dm->getCached<bool>("Strato/777/mcp/vs_fpa");
    bool vsOpen = dm->getCached<bool>("Strato/777/mcp/vshold");

    data.vsMode = isVsMode;
    data.fpaMode = !isVsMode;
    data.vsIndication = isVsMode;
    data.fpaIndication = !isVsMode;
    data.fpaComma = false;
    data.vsVerticalLine = vsOpen;

    if (vsOpen) {
        float vs = dm->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
        int absVs = std::abs(static_cast<int>(std::round(vs)));
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(4) << absVs;
        data.verticalSpeed = ss.str();
        data.vsSign = (vs >= 0);
    } else {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAValue;
    }

    // EFIS baro
    auto baroModes = dm->getCached<std::vector<float>>("Strato/777/baro_mode");
    auto altStd = dm->getCached<std::vector<float>>("Strato/777/displays/alt_std");

    for (int i = 0; i < 2; i++) {
        bool isCaptain = (i == 0);
        const char *baroRef = isCaptain
                                ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot"
                                : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";

        float baroInhg = dm->getCached<float>(baroRef);
        bool baroIsHpa = baroModes.size() > (size_t) i && baroModes[i] > 0.5f;
        bool isStd = altStd.size() > (size_t) i && altStd[i] > 0.5f;

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = !baroIsHpa,
            .isStd = isStd,
        };

        if (!isStd && baroInhg > 0) {
            value.setBaro(baroInhg, !baroIsHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void Strato77WFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    // Array element write: "array_set:DATAREF:INDEX"
    if (phase == xplm_CommandBegin && button->dataref.rfind("array_set:", 0) == 0) {
        std::string rest = button->dataref.substr(10);
        size_t colon = rest.rfind(':');
        if (colon != std::string::npos) {
            std::string ref = rest.substr(0, colon);
            int index = std::stoi(rest.substr(colon + 1));
            auto current = dm->get<std::vector<float>>(ref.c_str());
            if (index < (int) current.size()) {
                current[index] = static_cast<float>(button->value);
                dm->set<std::vector<float>>(ref.c_str(), current);
            }
        }
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_PHASED) {
        dm->executeCommand(button->dataref.c_str(), phase);
    } else if (button->datarefType != FCUEfisDatarefType::SET_VALUE) {
        // Default: fire as command on begin only
        if (phase == xplm_CommandBegin) {
            dm->executeCommand(button->dataref.c_str());
        }
    }
}

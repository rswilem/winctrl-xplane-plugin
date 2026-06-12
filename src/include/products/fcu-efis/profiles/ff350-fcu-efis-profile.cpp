#include "ff350-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

FF350FCUEfisProfile::FF350FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/SupplLightLevelRehostats", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");

        uint8_t target = hasPower ? brightness[0] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);

        uint8_t ledBrightness = Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 0 ? 60 : 255;
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? ledBrightness : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? ledBrightness : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? ledBrightness : 0);

        constexpr uint8_t minimumScreenBrightness = 64;
        uint8_t screenBrightness = hasPower ? minimumScreenBrightness + brightness[1] * (255 - minimumScreenBrightness) : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP1Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP2Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/ATHRmode", [this, product](int mode) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, mode > 0 || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/LOCilluminated", [this, product](bool illuminated) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, illuminated || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/APPRilluminated", [this, product](bool illuminated) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, illuminated || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/APVerticalMode", [this, product](int vsMode) {
        bool expedEnabled = vsMode >= 0 && vsMode & 0b00010000;
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, expedEnabled || isAnnunTest() ? 1 : 0);
    }, this);

    // Monitor EFIS Right (Captain) LED states
    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FD2Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, engaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ILSonFO", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISR_LS_GREEN, on || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowCSTRFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowWPTFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowVORDFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowNDBFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowARPTFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    // Monitor EFIS Left (First Officer) LED states
    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FD1Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, engaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ILSonCapt", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISL_LS_GREEN, on || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowCSTRCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowWPTCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowVORDCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowNDBCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowARPTCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, show || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this](int annunMode) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/SupplLightLevelRehostats");

        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP1Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP2Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ATHRmode");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/LOCilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/APPRilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/altitude_hold_armed"); //OK????

        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/FD2Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ILSonFO");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowCSTRFO");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowWPTFO");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowVORDFO");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowNDBFO");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowARPTFO");

        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/FD1Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ILSonCapt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowCSTRCapt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowWPTCapt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowVORDCapt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowNDBCapt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/NDShowARPTCapt");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/SupplLightLevelRehostats");
    }, this);
}

bool FF350FCUEfisProfile::IsEligible() {
    return (
        (Dataref::getInstance()->exists("AirbusFBW/FCUAvail")) &&
        (Dataref::getInstance()->exists("1-sim/fcu/ndZoomLeft/switch")) // exclude ToLiss aircrafts
    );
}

const std::vector<std::string> &FF350FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "AirbusFBW/FCUAvail",
        "AirbusFBW/AnnunMode",

        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "AirbusFBW/SPDmanaged",
        "AirbusFBW/SPDdashed",

        "sim/cockpit/autopilot/heading_mag",
        "AirbusFBW/HDGmanaged",
        "AirbusFBW/HDGdashed",

        "sim/cockpit/autopilot/altitude",
        "AirbusFBW/ALTmanaged",

        "sim/cockpit/autopilot/vertical_velocity",
        "AirbusFBW/VSdashed",

        "sim/cockpit/autopilot/airspeed_is_mach",
        "AirbusFBW/HDGTRKmode",

        "AirbusFBW/AP1Engage",
        "AirbusFBW/AP2Engage",

        "AirbusFBW/BaroStdCapt",
        "AirbusFBW/BaroUnitCapt",
        "AirbusFBW/BaroStdFO",
        "AirbusFBW/BaroUnitFO",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FF350FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {

        // MCP -------------------------------------------------------------------------------
        {0, {"MACH", "toliss_airbus/ias_mach_button_push"}},
        {1, {"LOC", "toliss_airbus/loc_push"}},
        {2, {"TRK", "toliss_airbus/hdgtrk_button_push"}},
        {3, {"AP1", "toliss_airbus/ap1_push"}},
        {4, {"AP2", "toliss_airbus/ap2_push"}},
        {5, {"A/THR", "sim/autopilot/autothrottle"}},
        {6, {"EXPED", "sim/autopilot/altitude_hold"}}, //'EXPED' does not exit on A350 --> 'ALT' button
        {7, {"METRIC", "toliss_airbus/metric_alt_button_push"}},
        {8, {"APPR", "sim/autopilot/approach"}},

        {9, {"SPD DEC", "1-sim/comm/spdRoteryDN"}},
        {10, {"SPD INC", "1-sim/comm/spdRoteryUP"}},
        {11, {"SPD PUSH", "toliss_airbus/spd_push"}},
        {12, {"SPD PULL", "toliss_airbus/spd_pull"}},

        {13, {"HDG DEC", "1-sim/fcu/hdgRoteryDN"}},
        {14, {"HDG INC", "1-sim/fcu/hdgRoteryUP"}},
        {15, {"HDG PUSH", "sim/autopilot/NAV"}},
        {16, {"HDG PULL", "sim/autopilot/heading"}},

        {17, {"ALT DEC", "1-sim/comm/altRoteryDN"}},
        {18, {"ALT INC", "1-sim/comm/altRoteryUP"}},
        {19, {"ALT PUSH", "sim/autopilot/vnav"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_arm"}},

        {21, {"VS DEC", "1-sim/comm/vviRoteryDN"}},
        {22, {"VS INC", "1-sim/comm/vviRoteryUP"}},
        {23, {"VS PUSH", "toliss_airbus/vs_push"}},
        {24, {"VS PULL", "sim/autopilot/vertical_speed"}},

        {25, {"ALT 100", "1-sim/fcu/altModeSwitch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {26, {"ALT 1000", "1-sim/fcu/altModeSwitch", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // Buttons 27-31 reserved

        // EFIS CAPT -------------------------------------------------------------------------
        {32, {"L_FD", "toliss_airbus/fd1_push"}},
        {33, {"L_LS", "1-sim/144/button", FCUEfisDatarefType::TOGGLE_VALUE, 0}}, // FlightFactor logic !!

        {34, {"L_CSTR", "toliss_airbus/dispcommands/CaptCstrPushButton"}},
        {35, {"L_WPT", "toliss_airbus/dispcommands/CaptWptPushButton"}},
        {36, {"L_VOR.D", "toliss_airbus/dispcommands/CaptVorDPushButton"}},
        {37, {"L_NDB", "toliss_airbus/dispcommands/CaptNdbPushButton"}},
        {38, {"L_ARPT", "toliss_airbus/dispcommands/CaptArptPushButton"}},

        {39, {"L_STD PUSH", "1-sim/pres/pressLeftButton", FCUEfisDatarefType::TOGGLE_VALUE, 0}},
        {40, {"L_STD PULL", "1-sim/pres/pressLeftButton", FCUEfisDatarefType::TOGGLE_VALUE, 0}},
        {41, {"L_PRESS DEC", "1-sim/comm/pressLeftRotaryDN"}},
        {42, {"L_PRESS INC", "1-sim/comm/pressLeftRotaryUP"}},

        {43, {"L_inHg", "1-sim/pres/pressModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "1-sim/pres/pressModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},

        {45, {"L_MODE LS", "1-sim/fcu/ndModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "1-sim/fcu/ndModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "1-sim/fcu/ndModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {48, {"L_MODE ARC", "1-sim/fcu/ndModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {49, {"L_MODE PLAN", "1-sim/fcu/ndModeLeft/switch", FCUEfisDatarefType::SET_VALUE, 4.0}},

        {50, {"L_RANGE 10", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "1-sim/fcu/ndZoomLeft/switch", FCUEfisDatarefType::SET_VALUE, 5.0}},

        {56, {"L_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {57, {"L_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {58, {"L_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {59, {"L_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {60, {"L_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {61, {"L_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},

        // Buttons 62-63 reserved

        // EFIS DO --------------------------------------------------------------------------
        {64, {"R_FD", "toliss_airbus/fd2_push"}},
        {65, {"R_LS", "1-sim/166/button", FCUEfisDatarefType::TOGGLE_VALUE, 0}}, // FlightFactor logic !!

        {66, {"R_CSTR", "toliss_airbus/dispcommands/CoCstrPushButton"}},
        {67, {"R_WPT", "toliss_airbus/dispcommands/CoWptPushButton"}},
        {68, {"R_VOR.D", "toliss_airbus/dispcommands/CoVorDPushButton"}},
        {69, {"R_NDB", "toliss_airbus/dispcommands/CoNdbPushButton"}},
        {70, {"R_ARPT", "toliss_airbus/dispcommands/CoArptPushButton"}},

        {71, {"R_STD PUSH", "1-sim/pres/pressRightButton", FCUEfisDatarefType::TOGGLE_VALUE, 0}},
        {72, {"L_STD PULL", "1-sim/pres/pressRightButton", FCUEfisDatarefType::TOGGLE_VALUE, 0}},
        {73, {"R_PRESS DEC", "1-sim/comm/pressRightRotaryDN"}},
        {74, {"R_PRESS INC", "1-sim/comm/pressRightRotaryUP"}},

        {75, {"R_inHg", "1-sim/pres/pressModeRight/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "1-sim/pres/pressModeRight/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},

        {77, {"R_MODE LS", "1-sim/fcu/ndModeRight/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "1-sim/fcu/ndModeRight/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "1-sim/fcu/ndModeRight/switch", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {80, {"R_MODE ARC", "1-sim/fcu/ndModeRight/switch", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {81, {"R_MODE PLAN", "1-sim/fcu/ndModeRight/switch", FCUEfisDatarefType::SET_VALUE, 4.0}},

        {82, {"R_RANGE 10", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "1-sim/fcu/ndZoomRight/switch", FCUEfisDatarefType::SET_VALUE, 5.0}},

        {88, {"R_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {89, {"R_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {90, {"R_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {91, {"R_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {92, {"R_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {93, {"R_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},

        // Buttons 94-95 reserved
    };
    return buttons;
}

void FF350FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("AirbusFBW/FCUAvail");
    data.displayTest = datarefManager->getCached<int>("AirbusFBW/AnnunMode") == 2;

    // Set managed mode indicators - using validated int datarefs (1 or 0)
    data.spdManaged = datarefManager->getCached<bool>("AirbusFBW/SPDmanaged");
    data.hdgManaged = datarefManager->getCached<bool>("AirbusFBW/HDGmanaged");
    data.altManaged = datarefManager->getCached<bool>("AirbusFBW/ALTmanaged");

    // Speed/Mach mode - using sim/cockpit/autopilot/airspeed_is_mach (int, 1 or 0)
    data.spdMach = datarefManager->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = datarefManager->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");

    if (speed > 0 && datarefManager->getCached<bool>("AirbusFBW/SPDdashed") == false) {
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
    if (heading >= 0 && datarefManager->getCached<bool>("AirbusFBW/HDGdashed") == false) {
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
    bool vsDashed = datarefManager->getCached<bool>("AirbusFBW/VSdashed");

    // HDG/TRK mode - using AirbusFBW/HDGTRKmode (int, HDG=0, TRK=1)
    data.headingTrk = datarefManager->getCached<bool>("AirbusFBW/HDGTRKmode");
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

        bool isStd = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroStdCapt" : "AirbusFBW/BaroStdFO");
        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroUnitCapt" : "AirbusFBW/BaroUnitFO");
        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .displayEnabled = datarefManager->getCached<bool>("AirbusFBW/FCUAvail"),
            .displayTest = datarefManager->getCached<int>("AirbusFBW/AnnunMode") == 2,
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

void FF350FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::SET_VALUE || button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE)) {
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

bool FF350FCUEfisProfile::isAnnunTest() {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2;
}

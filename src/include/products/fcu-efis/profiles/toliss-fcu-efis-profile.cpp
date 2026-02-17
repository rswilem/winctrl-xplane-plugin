#include "toliss-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

TolissFCUEfisProfile::TolissFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/SupplLightLevelRehostats", [product](std::vector<float> brightness) {
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
    });

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [this](int annunMode) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/SupplLightLevelRehostats");

        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP1Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP2Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ATHRmode");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/LOCilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/APPRilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/APVerticalMode");

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
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP1Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP2Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/ATHRmode", [this, product](int mode) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, mode > 0 || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/LOCilluminated", [this, product](bool illuminated) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, illuminated || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/APPRilluminated", [this, product](bool illuminated) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, illuminated || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/APVerticalMode", [this, product](int vsMode) {
        /*0=SRS, 1=CLB, 2=DES, 3=ALT CST*, 4=ALT CST, 6=G/S*, 7=G/S, 8=FINAL, 10=FLARE, 11=LAND; 101=OP CLB, 102=OP DES, 103=ALT*, 104=ALT, 105: ALT CRZ, 107=V/S or FPA, 112: EXP CLB, 113: EXP DES*/
        static const std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
        static const std::set<std::string> expedButtonIcaos = {"A318", "A319", "A320", "A321", "A19N", "A20N",
            "A21N"};
        bool expedButtonAvailable = expedButtonIcaos.count(icao) > 0;
        bool isExpedMode = expedButtonAvailable && (vsMode == 112 || vsMode == 113);
        bool isAltMode = !expedButtonAvailable && (vsMode == 104 || vsMode == 105);
        bool illuminated = isExpedMode || isAltMode;
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, illuminated || isAnnunTest() ? 1 : 0);
    });

    // Monitor EFIS Right (Captain) LED states
    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FD2Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, engaged || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ILSonFO", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISR_LS_GREEN, on || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowCSTRFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowWPTFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowVORDFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowNDBFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowARPTFO", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    // Monitor EFIS Left (First Officer) LED states
    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FD1Engage", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, engaged || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ILSonCapt", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISL_LS_GREEN, on || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowCSTRCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowWPTCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowVORDCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowNDBCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, show || isAnnunTest() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/NDShowARPTCapt", [this, product](bool show) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, show || isAnnunTest() ? 1 : 0);
    });
}

TolissFCUEfisProfile::~TolissFCUEfisProfile() {
    Dataref::getInstance()->unbind("AirbusFBW/SupplLightLevelRehostats");
    Dataref::getInstance()->unbind("AirbusFBW/FCUAvail");
    Dataref::getInstance()->unbind("AirbusFBW/AnnunMode");

    // Unbind FCU datarefs
    Dataref::getInstance()->unbind("AirbusFBW/AP1Engage");
    Dataref::getInstance()->unbind("AirbusFBW/AP2Engage");
    Dataref::getInstance()->unbind("AirbusFBW/ATHRmode");
    Dataref::getInstance()->unbind("AirbusFBW/LOCilluminated");
    Dataref::getInstance()->unbind("AirbusFBW/APPRilluminated");
    Dataref::getInstance()->unbind("AirbusFBW/APVerticalMode");

    // Unbind EFIS Right datarefs
    Dataref::getInstance()->unbind("AirbusFBW/FD2Engage");
    Dataref::getInstance()->unbind("AirbusFBW/ILSonFO");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowCSTRFO");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowWPTFO");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowVORDFO");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowNDBFO");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowARPTFO");

    // Unbind EFIS Left datarefs
    Dataref::getInstance()->unbind("AirbusFBW/FD1Engage");
    Dataref::getInstance()->unbind("AirbusFBW/ILSonCapt");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowCSTRCapt");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowWPTCapt");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowVORDCapt");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowNDBCapt");
    Dataref::getInstance()->unbind("AirbusFBW/NDShowARPTCapt");
}

bool TolissFCUEfisProfile::IsEligible() {
    return ((Dataref::getInstance()->exists("AirbusFBW/FCUAvail")) &&
            (Dataref::getInstance()->exists("AirbusFBW/NDrangeCapt"))); // exclude the FFA350
}

const std::vector<std::string> &TolissFCUEfisProfile::displayDatarefs() const {
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
        "toliss_airbus/pfdoutputs/general/ap_altitude_reference",
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

const std::unordered_map<uint16_t, FCUEfisButtonDef> &TolissFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", "toliss_airbus/ias_mach_button_push"}},
        {1, {"LOC", "AirbusFBW/LOCbutton"}},
        {2, {"TRK", "toliss_airbus/hdgtrk_button_push"}},
        {3, {"AP1", "AirbusFBW/AP1Engage", FCUEfisDatarefType::TOGGLE_VALUE}},
        {4, {"AP2", "AirbusFBW/AP2Engage", FCUEfisDatarefType::TOGGLE_VALUE}},
        {5, {"A/THR", "AirbusFBW/ATHRbutton"}},
        {6, {"EXPED", "AirbusFBW/EXPEDbutton"}},
        {7, {"METRIC", "toliss_airbus/metric_alt_button_push"}},
        {8, {"APPR", "AirbusFBW/APPRbutton"}},
        {9, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {11, {"SPD PUSH", "AirbusFBW/PushSPDSel"}},
        {12, {"SPD PULL", "AirbusFBW/PullSPDSel"}},
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "AirbusFBW/PushHDGSel"}},
        {16, {"HDG PULL", "AirbusFBW/PullHDGSel"}},
        {17, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {18, {"ALT INC", "sim/autopilot/altitude_up"}},
        {19, {"ALT PUSH", "AirbusFBW/PushAltitude"}},
        {20, {"ALT PULL", "AirbusFBW/PullAltitude"}},
        {21, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC", "sim/autopilot/vertical_speed_up"}},
        {23, {"VS PUSH", "AirbusFBW/PushVSSel"}},
        {24, {"VS PULL", "AirbusFBW/PullVSSel"}},
        {25, {"ALT 100", "AirbusFBW/ALT100_1000", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {26, {"ALT 1000", "AirbusFBW/ALT100_1000", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // Buttons 27-31 reserved

        {32, {"L_FD", "toliss_airbus/fd1_push"}},
        {33, {"L_LS", "toliss_airbus/dispcommands/CaptLSButtonPush"}},
        {34, {"L_CSTR", "toliss_airbus/dispcommands/CaptCstrPushButton"}},
        {35, {"L_WPT", "toliss_airbus/dispcommands/CaptWptPushButton"}},
        {36, {"L_VOR.D", "toliss_airbus/dispcommands/CaptVorDPushButton"}},
        {37, {"L_NDB", "toliss_airbus/dispcommands/CaptNdbPushButton"}},
        {38, {"L_ARPT", "toliss_airbus/dispcommands/CaptArptPushButton"}},
        {39, {"L_STD PUSH", "toliss_airbus/capt_baro_push"}},
        {40, {"L_STD PULL", "toliss_airbus/capt_baro_pull"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "AirbusFBW/BaroUnitCapt", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "AirbusFBW/BaroUnitCapt", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {45, {"L_MODE LS", "AirbusFBW/NDmodeCapt", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "AirbusFBW/NDmodeCapt", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "AirbusFBW/NDmodeCapt", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {48, {"L_MODE ARC", "AirbusFBW/NDmodeCapt", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {49, {"L_MODE PLAN", "AirbusFBW/NDmodeCapt", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {50, {"L_RANGE 10", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "AirbusFBW/NDrangeCapt", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {56, {"L_1 ADF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {57, {"L_1 OFF", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {58, {"L_1 VOR", "sim/cockpit2/EFIS/EFIS_1_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {59, {"L_2 ADF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {60, {"L_2 OFF", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {61, {"L_2 VOR", "sim/cockpit2/EFIS/EFIS_2_selection_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        // Buttons 62-63 reserved

        {64, {"R_FD", "toliss_airbus/fd2_push"}},
        {65, {"R_LS", "toliss_airbus/dispcommands/CoLSButtonPush"}},
        {66, {"R_CSTR", "toliss_airbus/dispcommands/CoCstrPushButton"}},
        {67, {"R_WPT", "toliss_airbus/dispcommands/CoWptPushButton"}},
        {68, {"R_VOR.D", "toliss_airbus/dispcommands/CoVorDPushButton"}},
        {69, {"R_NDB", "toliss_airbus/dispcommands/CoNdbPushButton"}},
        {70, {"R_ARPT", "toliss_airbus/dispcommands/CoArptPushButton"}},
        {71, {"R_STD PUSH", "toliss_airbus/copilot_baro_push"}},
        {72, {"R_STD PULL", "toliss_airbus/copilot_baro_pull"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "AirbusFBW/BaroUnitFO", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "AirbusFBW/BaroUnitFO", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {77, {"R_MODE LS", "AirbusFBW/NDmodeFO", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "AirbusFBW/NDmodeFO", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "AirbusFBW/NDmodeFO", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {80, {"R_MODE ARC", "AirbusFBW/NDmodeFO", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {81, {"R_MODE PLAN", "AirbusFBW/NDmodeFO", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {82, {"R_RANGE 10", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "AirbusFBW/NDrangeFO", FCUEfisDatarefType::SET_VALUE, 5.0}},
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

void TolissFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("AirbusFBW/FCUAvail");
    data.displayTest = isAnnunTest(true);

    // Set managed mode indicators
    data.spdManaged = datarefManager->getCached<bool>("AirbusFBW/SPDmanaged");
    data.hdgManaged = datarefManager->getCached<bool>("AirbusFBW/HDGmanaged");
    data.altManaged = datarefManager->getCached<bool>("AirbusFBW/ALTmanaged");

    // Speed/Mach mode
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

    // Format FCU heading display
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

    // Format FCU altitude display
    float altitude = datarefManager->getCached<float>("toliss_airbus/pfdoutputs/general/ap_altitude_reference");
    if (altitude >= 0) {
        int altInt = static_cast<int>(altitude);
        std::stringstream ss;
        // Always show full altitude value
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    // Format vertical speed display
    float vs = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    bool vsDashed = datarefManager->getCached<bool>("AirbusFBW/VSdashed");

    // HDG/TRK mode
    data.hdgTrk = datarefManager->getCached<bool>("AirbusFBW/HDGTRKmode");
    data.vsMode = !data.hdgTrk; // VS mode when HDG mode
    data.fpaMode = data.hdgTrk; // FPA mode when TRK mode

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
    data.latMode = true;

    for (int i = 0; i < 2; i++) {
        bool isCaptain = i == 0;

        bool isStd = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroStdCapt" : "AirbusFBW/BaroStdFO");
        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroUnitCapt" : "AirbusFBW/BaroUnitFO");
        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .displayEnabled = datarefManager->getCached<bool>("AirbusFBW/FCUAvail"),
            .displayTest = isAnnunTest(true),
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

void TolissFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;
        bool isStd = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroStdCapt" : "AirbusFBW/BaroStdFO");
        if (isStd) {
            return;
        }

        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "AirbusFBW/BaroUnitCapt" : "AirbusFBW/BaroUnitFO");
        const char *datarefName = isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";
        float baroValue = datarefManager->getCached<float>(datarefName);
        bool increase = button->value > 0;

        if (isBaroHpa) {
            float hpaValue = baroValue * 33.8639f;
            hpaValue += increase ? 1.0f : -1.0f;
            baroValue = hpaValue / 33.8639f;
        } else {
            baroValue += increase ? 0.01f : -0.01f;
        }

        datarefManager->set<float>(datarefName, baroValue);
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::SET_VALUE || button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE)) {
        bool wantsToggle = button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE;

        if (wantsToggle) {
            int currentValue = datarefManager->get<int>(button->dataref.c_str());
            int newValue = currentValue ? 0 : 1;
            datarefManager->set<int>(button->dataref.c_str(), newValue);
        } else {
            datarefManager->set<float>(button->dataref.c_str(), button->value);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

bool TolissFCUEfisProfile::isAnnunTest(bool allowEssentialBusPowerOnly) {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2 && (allowEssentialBusPowerOnly ? Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail") : Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on"));
}

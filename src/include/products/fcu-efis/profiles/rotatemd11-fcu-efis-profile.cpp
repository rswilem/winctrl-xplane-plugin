#include "rotatemd11-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

RotateMD11FCUEfisProfile::RotateMD11FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd", [this, product](bool hasPower) {
        float panelBrt = hasPower ? Dataref::getInstance()->getCached<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio") : 0.0f;
        uint8_t backlight = hasPower ? static_cast<uint8_t>(std::clamp(panelBrt, 0.0f, 1.0f) * 255) : 0;
        uint8_t screenBrightness = hasPower ? 200 : 0;
        uint8_t ledBrightness = hasPower ? 255 : 0;

        product->setLedBrightness(FCUEfisLed::BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, backlight);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, backlight);

        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, ledBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, ledBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, ledBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio", [](float) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/afs_appr_engaged", [this, product](bool engaged) {
        bool landArmed = Dataref::getInstance()->getCached<bool>("Rotate/aircraft/systems/afs_land_armed");
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, engaged || isAnnunTest() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, landArmed || engaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/afs_land_armed", [this, product](bool armed) {
        bool apprEngaged = Dataref::getInstance()->getCached<bool>("Rotate/aircraft/systems/afs_appr_engaged");
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, armed || apprEngaged || isAnnunTest() ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/annun_test_signal", [this, product](int) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/afs_ap_engaged");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/afs_cws_engaged");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/afs_at_engaged");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/afs_appr_engaged");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/afs_land_armed");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>("Rotate/aircraft/systems/gcp_baro_inhg_hpa_mode", [this, product](const std::vector<int> &mode) {
        if (mode.size() >= 2) {
            isBaroHpaCapt = (mode[0] == 1);
            isBaroHpaFo = (mode[1] == 1);
            product->updateDisplays();
        }
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/controls/baro_qfe_qnh_l", [this, product](bool isQnh) {
        isQfeCapt = !isQnh;
        product->updateDisplays();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/controls/baro_qfe_qnh_r", [this, product](bool isQnh) {
        isQfeFo = !isQnh;
        product->updateDisplays();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/du_1_show_cross", [product](int) {
        product->forceStateSync();
    }, this);
}

RotateMD11FCUEfisProfile::~RotateMD11FCUEfisProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool RotateMD11FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/systems/gcp_alt_presel_ft");
}

bool RotateMD11FCUEfisProfile::isAnnunTest() {
    return Dataref::getInstance()->get<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
}

const std::vector<std::string> &RotateMD11FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "Rotate/aircraft/systems/elec_dc_batt_bus_pwrd",

        "Rotate/aircraft/systems/gcp_spd_presel_ias",
        "Rotate/aircraft/systems/gcp_spd_presel_mach",
        "Rotate/aircraft/systems/gcp_active_ias_mach_mode",
        "Rotate/aircraft/systems/afs_fms_spd_engaged",

        "Rotate/aircraft/systems/gcp_hdg_presel_deg",
        "Rotate/aircraft/systems/gcp_hdg_trk_presel_set",
        "Rotate/aircraft/systems/gcp_hdg_trk_mode",

        "Rotate/aircraft/systems/gcp_alt_presel_ft",
        "Rotate/aircraft/systems/gcp_alt_ft_meter_mode",

        "Rotate/aircraft/systems/gcp_vs_fpa_mode",
        "Rotate/aircraft/systems/gcp_vs_sel_fpm",
        "Rotate/aircraft/systems/gcp_fpa_sel_deg",
        "Rotate/aircraft/systems/gcp_pitch_sel_set",

        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &RotateMD11FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU main buttons
        {0, {"MACH", "Rotate/aircraft/controls_c/fgs_ias_mach"}},
        {1, {"LOC", "Rotate/aircraft/controls_c/fgs_nav", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"TRK", "custom_hdgtrk"}},
        {3, {"AP1", "Rotate/aircraft/controls_c/fgs_autoflight", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"AP2", "Rotate/aircraft/controls_c/fgs_autoflight", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"A/THR", ""}},
        {6, {"EXPED", ""}},
        {7, {"METRIC", "Rotate/aircraft/controls_c/fgs_ft_m"}},
        {8, {"APPR", "Rotate/aircraft/controls_c/fgs_appr_land", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // SPD encoder
        {9, {"SPD DEC", "Rotate/aircraft/controls_c/fgs_spd_sel_dn"}},
        {10, {"SPD INC", "Rotate/aircraft/controls_c/fgs_spd_sel_up"}},
        {11, {"SPD PUSH", "Rotate/aircraft/controls_c/fgs_spd_sel_mode_up", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"SPD PULL", "Rotate/aircraft/controls_c/fgs_spd_sel_mode_dn", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // HDG encoder
        {13, {"HDG DEC", "Rotate/aircraft/controls_c/fgs_hdg_sel_dn"}},
        {14, {"HDG INC", "Rotate/aircraft/controls_c/fgs_hdg_sel_up"}},
        {15, {"HDG PUSH", "Rotate/aircraft/controls_c/fgs_hdg_mode_sel_up", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {16, {"HDG PULL", "Rotate/aircraft/controls_c/fgs_hdg_mode_sel_dn", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // ALT encoder
        {17, {"ALT DEC", "Rotate/aircraft/controls_c/fgs_alt_sel_dn"}},
        {18, {"ALT INC", "Rotate/aircraft/controls_c/fgs_alt_sel_up"}},
        {19, {"ALT PUSH", "Rotate/aircraft/controls_c/fgs_alt_mode_sel_up", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {20, {"ALT PULL", "Rotate/aircraft/controls_c/fgs_alt_mode_sel_dn", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // VS encoder
        {21, {"VS DEC", "Rotate/aircraft/controls_c/fgs_pitch_sel_up"}},
        {22, {"VS INC", "Rotate/aircraft/controls_c/fgs_pitch_sel_dn"}},
        {23, {"VS PUSH", ""}},
        {24, {"VS PULL", "Rotate/aircraft/controls_c/fgs_prof", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},

        // Buttons 25-31 reserved

        // EFIS Left (Captain)
        {32, {"L_FD", ""}},
        {33, {"L_LS", ""}},
        {34, {"L_CSTR", "Rotate/aircraft/controls_c/eis_show_data_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {35, {"L_WPT", "Rotate/aircraft/controls_c/eis_show_wpt_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {36, {"L_VOR.D", "Rotate/aircraft/controls_c/eis_show_vor_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {37, {"L_NDB", "Rotate/aircraft/controls_c/eis_show_traffic_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {38, {"L_ARPT", "Rotate/aircraft/controls_c/eis_show_apt_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {39, {"L_STD PUSH", "Rotate/aircraft/controls_c/baro_std_set_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {40, {"L_STD PULL", "Rotate/aircraft/controls_c/baro_std_set_l", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {41, {"L_PRESS DEC", "Rotate/aircraft/controls_c/baro_set_l_dn"}},
        {42, {"L_PRESS INC", "Rotate/aircraft/controls_c/baro_set_l_up"}},
        {43, {"L_inHg", "custom_baro_units_l_inhg"}},
        {44, {"L_hPa", "custom_baro_units_l_hpa"}},
        {45, {"L_MODE LS", "Rotate/aircraft/controls_c/eis_mode_appr_l"}},
        {46, {"L_MODE VOR", "Rotate/aircraft/controls_c/eis_mode_vor_l"}},
        {47, {"L_MODE NAV", "Rotate/aircraft/controls_c/eis_mode_map_l"}},
        {48, {"L_MODE ARC", "Rotate/aircraft/controls_c/eis_mode_tcas_l"}},
        {49, {"L_MODE PLAN", "Rotate/aircraft/controls_c/eis_mode_plan_l"}},
        {50, {"L_RANGE 10", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {51, {"L_RANGE 20", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {52, {"L_RANGE 40", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {53, {"L_RANGE 80", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
        {54, {"L_RANGE 160", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
        {55, {"L_RANGE 320", "sim/cockpit2/EFIS/map_range,Rotate/aircraft/controls_c/eis_range_decr_l,Rotate/aircraft/controls_c/eis_range_incr_l", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
        {56, {"L_1 ADF", "custom_vor_adf_l_adf1"}},
        {57, {"L_1 OFF", "custom_vor_adf_l_vor1_off"}},
        {58, {"L_1 VOR", "custom_vor_adf_l_vor1_on"}},
        {59, {"L_2 ADF", "custom_vor_adf_l_adf2"}},
        {60, {"L_2 OFF", "custom_vor_adf_l_vor2_off"}},
        {61, {"L_2 VOR", "custom_vor_adf_l_vor2_on"}},
        // Buttons 62-63 reserved

        // EFIS Right (First Officer)
        {64, {"R_FD", ""}},
        {65, {"R_LS", ""}},
        {66, {"R_CSTR", "Rotate/aircraft/controls_c/eis_show_data_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {67, {"R_WPT", "Rotate/aircraft/controls_c/eis_show_wpt_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {68, {"R_VOR.D", "Rotate/aircraft/controls_c/eis_show_vor_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {69, {"R_NDB", "Rotate/aircraft/controls_c/eis_show_traffic_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {70, {"R_ARPT", "Rotate/aircraft/controls_c/eis_show_apt_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {71, {"R_STD PUSH", "Rotate/aircraft/controls_c/baro_std_set_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {72, {"R_STD PULL", "Rotate/aircraft/controls_c/baro_std_set_r", FCUEfisDatarefType::EXECUTE_CMD_PHASED}},
        {73, {"R_PRESS DEC", "Rotate/aircraft/controls_c/baro_set_r_dn"}},
        {74, {"R_PRESS INC", "Rotate/aircraft/controls_c/baro_set_r_up"}},
        {75, {"R_inHg", "custom_baro_units_r_inhg"}},
        {76, {"R_hPa", "custom_baro_units_r_hpa"}},
        {77, {"R_MODE LS", "Rotate/aircraft/controls_c/eis_mode_appr_r"}},
        {78, {"R_MODE VOR", "Rotate/aircraft/controls_c/eis_mode_vor_r"}},
        {79, {"R_MODE NAV", "Rotate/aircraft/controls_c/eis_mode_map_r"}},
        {80, {"R_MODE ARC", "Rotate/aircraft/controls_c/eis_mode_tcas_r"}},
        {81, {"R_MODE PLAN", "Rotate/aircraft/controls_c/eis_mode_plan_r"}},
        {82, {"R_RANGE 10", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {83, {"R_RANGE 20", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {84, {"R_RANGE 40", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {85, {"R_RANGE 80", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
        {86, {"R_RANGE 160", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
        {87, {"R_RANGE 320", "sim/cockpit2/EFIS/map_range_copilot,Rotate/aircraft/controls_c/eis_range_decr_r,Rotate/aircraft/controls_c/eis_range_incr_r", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
        {88, {"R_1 VOR", "custom_vor_adf_r_vor1_on"}},
        {89, {"R_1 OFF", "custom_vor_adf_r_vor1_off"}},
        {90, {"R_1 ADF", "custom_vor_adf_r_adf1"}},
        {91, {"R_2 VOR", "custom_vor_adf_r_vor2_on"}},
        {92, {"R_2 OFF", "custom_vor_adf_r_vor2_off"}},
        {93, {"R_2 ADF", "custom_vor_adf_r_adf2"}},
        // Buttons 94-95 reserved
    };

    return buttons;
}

void RotateMD11FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dataref = Dataref::getInstance();

    data.displayEnabled = dataref->getCached<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    data.displayTest = false;

    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::LevelChangeHeader;

    data.headingLat = false;
    data.spdManaged = false;
    data.hdgManaged = false;
    data.altManaged = false;

    if (!data.displayEnabled) {
        return;
    }

    // Speed
    bool fmsSpdEngaged = dataref->getCached<int>("Rotate/aircraft/systems/afs_fms_spd_engaged") == 1;
    if (fmsSpdEngaged) {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::SpeedMachHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::SpeedMachValue;
    } else {
        int iasMachMode = dataref->getCached<int>("Rotate/aircraft/systems/gcp_active_ias_mach_mode");
        data.spdMach = (iasMachMode != 0);

        float speed = data.spdMach
                        ? dataref->getCached<float>("Rotate/aircraft/systems/gcp_spd_presel_mach")
                        : dataref->getCached<float>("Rotate/aircraft/systems/gcp_spd_presel_ias");

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
    }

    // Heading
    int hdgTrkSel = dataref->getCached<int>("Rotate/aircraft/systems/gcp_hdg_trk_presel_set");
    if (hdgTrkSel != 1) {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::HeadingTrackHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::HeadingTrackValue;
    } else {
        int hdgTrkMode = dataref->getCached<int>("Rotate/aircraft/systems/gcp_hdg_trk_mode");
        data.headingHdg = (hdgTrkMode == 0);
        data.headingTrk = !data.headingHdg;

        float heading = dataref->getCached<float>("Rotate/aircraft/systems/gcp_hdg_presel_deg");
        int hdgDisplay = static_cast<int>(heading) % 360;
        if (hdgDisplay < 0) {
            hdgDisplay += 360;
        }
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    }

    // Altitude
    dataref->getCached<int>("Rotate/aircraft/systems/gcp_alt_ft_meter_mode");
    float altitude = dataref->getCached<float>("Rotate/aircraft/systems/gcp_alt_presel_ft");
    if (altitude >= 0) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << static_cast<int>(std::round(altitude));
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    // Vertical speed / FPA
    int vsFpaMode = dataref->getCached<int>("Rotate/aircraft/systems/gcp_vs_fpa_mode");
    data.vsMode = (vsFpaMode == 0);
    data.fpaMode = !data.vsMode;

    if (data.fpaMode) {
        float fpa = dataref->getCached<float>("Rotate/aircraft/systems/gcp_fpa_sel_deg");
        float absFpa = std::abs(fpa);
        int fpaTenths = static_cast<int>(std::round(absFpa * 10));

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << fpaTenths << "  ";
        data.verticalSpeed = ss.str();

        data.fpaComma = true;
        data.vsSign = (fpa >= 0);
        data.vsHorizontalLine = true;
        data.vsVerticalLine = true;
        data.vsIndication = false;
        data.fpaIndication = true;
    } else {
        bool pitchSelSet = dataref->getCached<bool>("Rotate/aircraft/systems/gcp_pitch_sel_set");
        float vs = dataref->getCached<float>("Rotate/aircraft/systems/gcp_vs_sel_fpm");
        int absVs = std::abs(static_cast<int>(std::round(vs)));

        if (pitchSelSet) {
            std::stringstream ss;
            ss << std::setw(4) << ((absVs < 10 ? "0" : "") + std::to_string(absVs));
            data.verticalSpeed = ss.str();
            data.vsSign = (vs >= 0);
            data.vsHorizontalLine = true;
            data.vsVerticalLine = true;
        } else {
            data.verticalSpeed = "----";
            data.vsSign = false;
            data.vsHorizontalLine = false;
            data.vsVerticalLine = false;
        }

        data.fpaComma = false;
        data.vsIndication = true;
        data.fpaIndication = false;
    }

    // EFIS baro displays
    for (int i = 0; i < 2; i++) {
        bool isCaptain = (i == 0);
        bool isBaroHpa = isCaptain ? isBaroHpaCapt : isBaroHpaFo;
        bool isQfe = isCaptain ? isQfeCapt : isQfeFo;
        float baroValue = dataref->getCached<float>(
            isCaptain
                ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot"
                : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = false,
            .isStd = false,
            .showQfe = isQfe,
        };

        if (baroValue > 0) {
            value.setBaro(baroValue, !isBaroHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void RotateMD11FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->dataref == "custom_hdgtrk") {
        if (phase == xplm_CommandBegin) {
            datarefManager->executeCommand("Rotate/aircraft/controls_c/fgs_hdg_trk");
            datarefManager->executeCommand("Rotate/aircraft/controls_c/fgs_vs_fpa");
        }
    } else if (phase == xplm_CommandBegin && button->dataref.rfind("custom_baro_units", 0) == 0) {
        bool isCapt = (button->dataref.find("_l_") != std::string::npos);
        bool wantHpa = (button->dataref.back() == 'a');
        if ((isCapt ? isBaroHpaCapt : isBaroHpaFo) != wantHpa) {
            datarefManager->executeCommand(isCapt ? "Rotate/aircraft/controls_c/baro_units_l" : "Rotate/aircraft/controls_c/baro_units_r");
        }
    } else if (button->dataref.rfind("custom_vor_adf", 0) == 0) {
        bool isCapt = (button->dataref.find("_l_") != std::string::npos);
        int idx = isCapt ? 0 : 1;
        std::string side = isCapt ? "_l" : "_r";
        const auto &dr = button->dataref;

        auto getState = [&](const char *datarefName) -> int {
            auto v = datarefManager->get<std::vector<int>>(datarefName);
            return (static_cast<int>(v.size()) > idx) ? v[idx] : 0;
        };

        auto toggleIfNeeded = [&](const char *stateDr, const char *cmdName, bool wantOn) {
            if ((getState(stateDr) != 0) != wantOn) {
                std::string cmd = std::string("Rotate/aircraft/controls_c/") + cmdName + side;
                datarefManager->executeCommand(cmd.c_str());
            }
        };

        // ON/ADF buttons: Begin = entering selector position (want on), End = leaving (want off)
        // OFF buttons: Begin only — also clears ADF for the same pointer
        if (dr.find("vor1_on") != std::string::npos) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_vor1_show", "eis_vor1_show", phase == xplm_CommandBegin);
        } else if (dr.find("vor1_off") != std::string::npos && phase == xplm_CommandBegin) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_vor1_show", "eis_vor1_show", false);
            toggleIfNeeded("Rotate/aircraft/systems/gcp_adf1_show", "eis_adf1_show", false);
        } else if (dr.find("vor2_on") != std::string::npos) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_vor2_show", "eis_vor2_show", phase == xplm_CommandBegin);
        } else if (dr.find("vor2_off") != std::string::npos && phase == xplm_CommandBegin) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_vor2_show", "eis_vor2_show", false);
            toggleIfNeeded("Rotate/aircraft/systems/gcp_adf2_show", "eis_adf2_show", false);
        } else if (dr.find("adf1") != std::string::npos) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_adf1_show", "eis_adf1_show", phase == xplm_CommandBegin);
        } else if (dr.find("adf2") != std::string::npos) {
            toggleIfNeeded("Rotate/aircraft/systems/gcp_adf2_show", "eis_adf2_show", phase == xplm_CommandBegin);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE_USING_COMMANDS) {
        const auto &spec = button->dataref;
        auto c1 = spec.find(',');
        auto c2 = (c1 != std::string::npos) ? spec.find(',', c1 + 1) : std::string::npos;
        if (c1 != std::string::npos && c2 != std::string::npos) {
            std::string datarefName = spec.substr(0, c1);
            std::string lowerCmd = spec.substr(c1 + 1, c2 - c1 - 1);
            std::string raiseCmd = spec.substr(c2 + 1);

            int vectorIdx = -1;
            size_t bo = datarefName.find('[');
            size_t bc = (bo != std::string::npos) ? datarefName.find(']', bo) : std::string::npos;
            if (bo != std::string::npos && bc != std::string::npos) {
                vectorIdx = datarefName[bo + 1] - '0';
                datarefName = datarefName.substr(0, bo);
            }

            float currentValue = 0;
            if (vectorIdx >= 0) {
                auto v = datarefManager->get<std::vector<int>>(datarefName.c_str());
                if (static_cast<int>(v.size()) > vectorIdx) {
                    currentValue = static_cast<float>(v[vectorIdx]);
                }
            } else {
                currentValue = datarefManager->get<float>(datarefName.c_str());
            }

            if (button->value < currentValue) {
                datarefManager->executeCommand(lowerCmd.c_str());
            } else if (button->value > currentValue) {
                datarefManager->executeCommand(raiseCmd.c_str());
            }
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

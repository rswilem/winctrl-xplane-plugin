#include "fps-ejets-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <XPLMUtilities.h>

namespace {
    constexpr const char *kAcAlive = "ssg/Elec/ac_alive";
    constexpr const char *kPanelBrightness = "SSG/EJET/LIGHTS/main_pnl_sw";
    constexpr const char *kAnnunTest = "SSG/EJET/LIGHTS/ann_test_sw";
    constexpr const char *kSpeedIsMach = "ssg/B748/MCP/mcp_ias_co_act";
    constexpr const char *kSpeedValue = "ssg/B748/MCP/mcp_ias_mach_act";
    constexpr const char *kSpeedFmsMan = "SSG/AP/spd_man_auto_sw";
    constexpr const char *kHeadingBug = "ssg/B748/MCP/mcp_heading_bug_act";
    constexpr const char *kAltitudeBug = "ssg/EJET/MCP/bug_sel_alt";
    constexpr const char *kVsWheel = "ssg/B748/MCP/mcp_alt_up_sw";
    constexpr const char *kLnavActive = "ssg/B748/MCP/mcp_lnav_act";
    constexpr const char *kVnavActive = "ssg/B748/MCP/mcp_vnav_act";
    constexpr const char *kBaroUnit = "SSG/PFD/baro_type_sw";
    constexpr const char *kBaroPilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot";
    constexpr const char *kBaroCopilot = "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot";

    bool isPushDataref(const std::string &dataref) {
        static const std::unordered_set<std::string> pushDatarefs = {
            "ssg/EJET/MCP/alt_meters_sw",
            "SSG/AP/hdg_sync_sw",
            "SSG/MCP/prev_nd_p_sw",
            "SSG/MCP/prev_nd_cp_sw",
        };

        return pushDatarefs.find(dataref) != pushDatarefs.end();
    }
}

FPSEjetsFCUEfisProfile::FPSEjetsFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    auto dm = Dataref::getInstance();

    dm->monitorExistingDataref<float>(kPanelBrightness, [this](float) {
        updateBacklight();
    },
        this);

    dm->monitorExistingDataref<float>(kAcAlive, [this](float) {
        updateBacklight();
        updateAnnunciators();
    },
        this);

    static const std::vector<std::string> annunciatorDatarefs = {
        kAnnunTest,
        "ssg/B748/MCP/mcp_a_cmd_act",
        "ssg/B748/MCP/mcp_at_arm_act",
        kLnavActive,
        "ssg/B748/MCP/mcp_vor_loc_act",
        "ssg/B748/MCP/mcp_app_act",
        "ssg/B748/MCP/mcp_plt_fd_act",
        "ssg/B748/MCP/mcp_cplt_fd_act",
        "ssg/EJET/HSI/hsi_pview_p_sw",
        "ssg/EJET/HSI/hsi_pview_cp_sw",
        "ssg/B748/ND/show_waypoint_pilot",
        "ssg/B748/ND/show_VOR_pilot",
        "ssg/B748/ND/show_airport_pilot",
        "ssg/B748/ND/show_waypoint_copilot",
        "ssg/B748/ND/show_VOR_copilot",
        "ssg/B748/ND/show_airport_copilot",
    };

    for (const auto &ref : annunciatorDatarefs) {
        dm->monitorExistingDataref<float>(ref.c_str(), [this](float) {
            updateAnnunciators();
        },
            this);
    }

    updateBacklight();
    updateAnnunciators();
}

bool FPSEjetsFCUEfisProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

void FPSEjetsFCUEfisProfile::updateBacklight() {
    auto dm = Dataref::getInstance();
    bool hasPower = dm->get<float>(kAcAlive) > 0.5f;
    float brightness = std::clamp(dm->get<float>(kPanelBrightness), 0.0f, 1.0f);
    uint8_t target = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;

    product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
    product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
    product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);

    uint8_t screenBrightness = hasPower ? 200 : 0;
    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

    uint8_t ledBrightness = hasPower ? 255 : 0;
    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, ledBrightness);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, ledBrightness);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, ledBrightness);

    product->forceStateSync();
}

void FPSEjetsFCUEfisProfile::updateAnnunciators() {
    auto dm = Dataref::getInstance();
    bool hasPower = dm->get<float>(kAcAlive) > 0.5f;
    bool test = dm->get<float>(kAnnunTest) > 0.5f;

    auto isOn = [dm](const char *ref) {
        return dm->get<float>(ref) > 0.5f;
    };

    auto led = [hasPower, test](bool on) -> uint8_t {
        return (hasPower && (test || on)) ? 1 : 0;
    };

    bool apEngaged = isOn("ssg/B748/MCP/mcp_a_cmd_act");
    product->setLedBrightness(FCUEfisLed::AP1_GREEN, led(apEngaged));
    product->setLedBrightness(FCUEfisLed::AP2_GREEN, led(apEngaged));
    product->setLedBrightness(FCUEfisLed::ATHR_GREEN, led(isOn("ssg/B748/MCP/mcp_at_arm_act")));
    product->setLedBrightness(FCUEfisLed::LOC_GREEN, led(isOn(kLnavActive) || isOn("ssg/B748/MCP/mcp_vor_loc_act")));
    product->setLedBrightness(FCUEfisLed::APPR_GREEN, led(isOn("ssg/B748/MCP/mcp_app_act")));

    product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, led(isOn("ssg/B748/MCP/mcp_plt_fd_act")));
    product->setLedBrightness(FCUEfisLed::EFISL_LS_GREEN, led(isOn("ssg/EJET/HSI/hsi_pview_p_sw")));
    product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, led(isOn("ssg/B748/ND/show_waypoint_pilot")));
    product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, led(isOn("ssg/B748/ND/show_VOR_pilot")));
    product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, led(isOn("ssg/B748/ND/show_airport_pilot")));

    product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, led(isOn("ssg/B748/MCP/mcp_cplt_fd_act")));
    product->setLedBrightness(FCUEfisLed::EFISR_LS_GREEN, led(isOn("ssg/EJET/HSI/hsi_pview_cp_sw")));
    product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, led(isOn("ssg/B748/ND/show_waypoint_copilot")));
    product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, led(isOn("ssg/B748/ND/show_VOR_copilot")));
    product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, led(isOn("ssg/B748/ND/show_airport_copilot")));
}

const std::vector<std::string> &FPSEjetsFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kAcAlive,
        kAnnunTest,
        kSpeedIsMach,
        kSpeedValue,
        kSpeedFmsMan,
        kHeadingBug,
        kAltitudeBug,
        kVsWheel,
        kLnavActive,
        kVnavActive,
        kBaroUnit,
        kBaroPilot,
        kBaroCopilot,
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FPSEjetsFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", kSpeedIsMach, FCUEfisDatarefType::TOGGLE_VALUE}},
        {1, {"LOC", "FPS/EJET/MCP/LNAV_COMM"}},
        {2, {"TRK/FPA", "FPS/EJET/MCP/FPA_COMM"}},
        {3, {"AP1", "FPS/EJET/MCP/AP_COMM"}},
        {4, {"AP2", "FPS/EJET/MCP/AP_COMM"}},
        {5, {"A/THR", "FPS/EJET/MCP/AT_COMM"}},
        {7, {"METRIC", "ssg/EJET/MCP/alt_meters_sw", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {8, {"APPR", "FPS/EJET/MCP/APP_COMM"}},
        {9, {"SPD DEC", "FPS/EJET/MCP/Speed_Down"}},
        {10, {"SPD INC", "FPS/EJET/MCP/Speed_UP"}},
        {11, {"SPD PUSH", kSpeedFmsMan, FCUEfisDatarefType::SET_VALUE, 1.0}},
        {12, {"SPD PULL", kSpeedFmsMan, FCUEfisDatarefType::SET_VALUE, 0.0}},
        {13, {"HDG DEC", "FPS/EJET/MCP/HDG_Down"}},
        {14, {"HDG INC", "FPS/EJET/MCP/HDG_UP"}},
        {15, {"HDG PUSH", "SSG/AP/hdg_sync_sw", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {16, {"HDG PULL", "FPS/EJET/MCP/HDG_COMM"}},
        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {19, {"ALT PUSH", "FPS/EJET/MCP/VNAV_COMM"}},
        {20, {"ALT PULL", "FPS/EJET/MCP/FLCH_COMM"}},
        {21, {"VS DEC", "FPS/EJET/MCP/VS_Down"}},
        {22, {"VS INC", "FPS/EJET/MCP/VS_UP"}},
        {23, {"VS PUSH", "FPS/EJET/MCP/ALT_COMM"}},
        {24, {"VS PULL", "FPS/EJET/MCP/VS_COMM"}},
        {25, {"ALT 100", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 100.0}},
        {26, {"ALT 1000", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 1000.0}},

        {32, {"L_FD", "FPS/EJET/MCP/FD_P_COMM"}},
        {33, {"L_LS", "SSG/MCP/prev_nd_p_sw", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {35, {"L_WPT", "ssg/B748/ND/show_waypoint_pilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {36, {"L_VOR.D", "ssg/B748/ND/show_VOR_pilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {38, {"L_ARPT", "ssg/B748/ND/show_airport_pilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {39, {"L_STD", kBaroPilot, FCUEfisDatarefType::SET_VALUE, 29.92}},
        {40, {"L_STD PULL", ""}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", kBaroUnit, FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", kBaroUnit, FCUEfisDatarefType::SET_VALUE, 1.0}},

        {45, {"L_MODE LS", "ssg/EJET/ND/nd_menu_upper_p", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {46, {"L_MODE VOR", "ssg/EJET/ND/nd_menu_upper_p", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "ssg/EJET/ND/nd_menu_upper_p", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {48, {"L_MODE ARC", "ssg/EJET/ND/nd_menu_upper_p", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {49, {"L_MODE PLAN", "ssg/EJET/ND/nd_menu_upper_p", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {50, {"L_RANGE 10", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "custom_range_pilot", FCUEfisDatarefType::SET_VALUE, 5.0}},

        {64, {"R_FD", "FPS/EJET/MCP/FD_CP_COMM"}},
        {65, {"R_LS", "SSG/MCP/prev_nd_cp_sw", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {67, {"R_WPT", "ssg/B748/ND/show_waypoint_copilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {68, {"R_VOR.D", "ssg/B748/ND/show_VOR_copilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {70, {"R_ARPT", "ssg/B748/ND/show_airport_copilot", FCUEfisDatarefType::TOGGLE_VALUE}},
        {71, {"R_STD", kBaroCopilot, FCUEfisDatarefType::SET_VALUE, 29.92}},
        {72, {"R_STD PULL", ""}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", kBaroUnit, FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", kBaroUnit, FCUEfisDatarefType::SET_VALUE, 1.0}},

        {77, {"R_MODE LS", "ssg/EJET/ND/nd_menu_upper_cp", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {78, {"R_MODE VOR", "ssg/EJET/ND/nd_menu_upper_cp", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "ssg/EJET/ND/nd_menu_upper_cp", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {80, {"R_MODE ARC", "ssg/EJET/ND/nd_menu_upper_cp", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {81, {"R_MODE PLAN", "ssg/EJET/ND/nd_menu_upper_cp", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {82, {"R_RANGE 10", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "custom_range_copilot", FCUEfisDatarefType::SET_VALUE, 5.0}},
    };

    return buttons;
}

void FPSEjetsFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dr = Dataref::getInstance();

    data.displayEnabled = dr->getCached<float>(kAcAlive) > 0.5f;
    data.displayTest = dr->getCached<float>(kAnnunTest) > 0.5f;
    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::LevelChangeHeader;

    data.spdMach = dr->getCached<float>(kSpeedIsMach) > 0.5f;
    float speed = dr->getCached<float>(kSpeedValue);
    if (speed > 0) {
        std::stringstream ss;
        if (data.spdMach) {
            int machHundredths = static_cast<int>(std::round(speed < 2.0f ? speed * 100.0f : speed));
            ss << std::setfill('0') << std::setw(3) << machHundredths;
        } else {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(std::round(speed));
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }
    data.spdManaged = dr->getCached<float>(kSpeedFmsMan) > 0.5f;

    float heading = dr->getCached<float>(kHeadingBug);
    int hdgDisplay = ((static_cast<int>(std::round(heading)) % 360) + 360) % 360;
    std::stringstream hdgSS;
    hdgSS << std::setfill('0') << std::setw(3) << hdgDisplay;
    data.heading = hdgSS.str();

    data.headingHdg = true;
    data.headingTrk = false;
    data.headingLat = false;
    data.hdgManaged = dr->getCached<float>(kLnavActive) > 0.5f;

    float altitude = dr->getCached<float>(kAltitudeBug) * 100.0f;
    if (altitude >= 0) {
        int altInt = static_cast<int>(std::round(altitude / 100.0f)) * 100;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }
    data.altManaged = dr->getCached<float>(kVnavActive) > 0.5f;

    data.vsMode = true;
    data.fpaMode = false;

    float vs = dr->getCached<float>(kVsWheel) * 100.0f;
    int vsInt = static_cast<int>(std::round(vs / 100.0f)) * 100;
    int absVs = std::abs(vsInt);

    std::stringstream vsSS;
    vsSS << std::setfill('0') << std::setw(2) << (absVs / 100) << "##";
    data.verticalSpeed = vsSS.str();
    data.vsSign = (vsInt >= 0);
    data.fpaComma = false;
    data.vsIndication = true;
    data.fpaIndication = false;
    data.vsVerticalLine = true;

    bool unitIsInHg = dr->getCached<float>(kBaroUnit) < 0.5f;
    auto makeEfis = [&](const char *baroRef) {
        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = unitIsInHg,
            .isStd = false,
        };

        float baro = dr->getCached<float>(baroRef);
        if (baro > 0) {
            value.setBaro(baro, unitIsInHg);
        }

        return value;
    };

    data.efisLeft = makeEfis(kBaroPilot);
    data.efisRight = makeEfis(kBaroCopilot);
}

void FPSEjetsFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (button->datarefType == FCUEfisDatarefType::SET_VALUE && isPushDataref(button->dataref)) {
        if (phase == xplm_CommandBegin) {
            dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
        } else if (phase == xplm_CommandEnd) {
            dm->set<float>(button->dataref.c_str(), 0.0f);
        }
        return;
    }

    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == "custom_set_altitude_mode") {
        altitudeIncrements = static_cast<int>(button->value);
    } else if (button->dataref == "custom_altitude") {
        const char *command = button->value > 0 ? "FPS/EJET/MCP/Alt_UP" : "FPS/EJET/MCP/Alt_Down";
        int steps = std::max(1, altitudeIncrements / 100);
        for (int i = 0; i < steps; i++) {
            dm->executeCommand(command);
        }
    } else if (button->dataref == "custom_range_pilot" || button->dataref == "custom_range_copilot") {
        bool isPilot = button->dataref == "custom_range_pilot";
        int &lastIndex = isPilot ? lastRangeIndexPilot : lastRangeIndexCopilot;
        int index = static_cast<int>(button->value);
        if (lastIndex >= 0 && index != lastIndex) {
            const char *command = index > lastIndex
                                      ? (isPilot ? "FPS/EJET/MISC/MAP_RANGE_UP_P_COMM" : "FPS/EJET/MISC/MAP_RANGE_UP_CP_COMM")
                                      : (isPilot ? "FPS/EJET/MISC/MAP_RANGE_DN_P_COMM" : "FPS/EJET/MISC/MAP_RANGE_DN_CP_COMM");
            int steps = std::abs(index - lastIndex);
            for (int i = 0; i < steps; i++) {
                dm->executeCommand(command);
            }
        }
        lastIndex = index;
    } else if (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO) {
        const char *baroRef = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT ? kBaroPilot : kBaroCopilot;
        bool isHpa = dm->getCached<float>(kBaroUnit) > 0.5f;
        float baro = dm->getCached<float>(baroRef);
        int direction = button->value > 0 ? 1 : -1;

        if (isHpa) {
            float hpa = std::round(baro * 33.8639f) + direction;
            baro = hpa / 33.8639f;
        } else {
            baro = (std::round(baro * 100.0f) + direction) / 100.0f;
        }

        dm->set<float>(baroRef, std::clamp(baro, 26.0f, 31.5f));
    } else if (button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        float current = dm->get<float>(button->dataref.c_str());
        dm->set<float>(button->dataref.c_str(), current > 0.5f ? 0.0f : 1.0f);
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    }
}

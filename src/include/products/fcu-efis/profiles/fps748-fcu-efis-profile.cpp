#include "fps748-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

FPS748FCUEfisProfile::FPS748FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    Dataref::getInstance()->monitorExistingDataref<float>((altPrefix + "/LGT/glaresheld_sw").c_str(), [product, altPrefix](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
        uint8_t target = hasPower ? brightness * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);

        uint8_t screenBrightness = hasPower ? 200 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? 255 : 0);

        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>((altPrefix + "/Elec/bus_1_powered").c_str(), [altPrefix](bool powered) {
        Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/LGT/glaresheld_sw").c_str());
    });

    // MCP engagement LEDs
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_a_cmd_act").c_str(), [product](float engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_b_cmd_act").c_str(), [product](float engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_at_arm_act").c_str(), [product](float armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, armed > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_vor_loc_act").c_str(), [product](float armed) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, armed > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_app_act").c_str(), [product](float armed) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, armed > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_lnav_act").c_str(), [product](float engaged) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, engaged > 0.5f ? 1 : 0);
    });

    // EFIS Left (captain) LEDs
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_plt_fd_act").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_waypoint_pilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_VOR_pilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_NDB_pilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_airport_pilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, on > 0.5f ? 1 : 0);
    });

    // EFIS Right (FO) LEDs
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_cplt_fd_act").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_waypoint_copilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_VOR_copilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_NDB_copilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, on > 0.5f ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/ND/show_airport_copilot").c_str(), [product](float on) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, on > 0.5f ? 1 : 0);
    });

    Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/Elec/bus_1_powered").c_str());
}

FPS748FCUEfisProfile::~FPS748FCUEfisProfile() {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    Dataref::getInstance()->unbind((altPrefix + "/LGT/glaresheld_sw").c_str());
    Dataref::getInstance()->unbind((altPrefix + "/Elec/bus_1_powered").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_a_cmd_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_b_cmd_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_at_arm_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_vor_loc_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_app_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_lnav_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_plt_fd_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_waypoint_pilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_VOR_pilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_NDB_pilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_airport_pilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/MCP/mcp_cplt_fd_act").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_waypoint_copilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_VOR_copilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_NDB_copilot").c_str());
    Dataref::getInstance()->unbind((prefix + "/B748/ND/show_airport_copilot").c_str());
}

bool FPS748FCUEfisProfile::IsSSGVersion() {
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

bool FPS748FCUEfisProfile::IsFPSVersion() {
    return Dataref::getInstance()->exists("FPS/748/simtime");
}

bool FPS748FCUEfisProfile::IsEligible() {
    return IsFPSVersion() || IsSSGVersion();
}

const std::vector<std::string> &FPS748FCUEfisProfile::displayDatarefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";
    static std::unordered_map<bool, std::vector<std::string>> cache;

    return cache.try_emplace(isSSG, std::vector<std::string>{
                                        altPrefix + "/Elec/bus_1_powered",
                                        prefix + "/B748/MCP/mcp_ias_mach_act",
                                        prefix + "/B748/systems/athr/MCPSPD_spdmach",
                                        prefix + "/B748/mcp/speed_is_blank",
                                        prefix + "/B748/MCP/mcp_heading_bug_act",
                                        prefix + "/B748/ND/tk_hdg_pfd",
                                        prefix + "/B748/MCP/mcp_alt_target_act",
                                        prefix + "/B748/MCP/mcp_vs_target_act",
                                        prefix + "/B748/mcp/vs_is_blank",
                                        "FPS/PFD/baro_now",
                                        "FPS/PFD/baro_now2",
                                        "FPS/PFD/baro_standard",
                                        "FPS/PFD/baro_standard2",
                                        "FPS/PFD/baro_type_sw",
                                        prefix + "/B748/ND/mode_pilot",
                                        prefix + "/B748/ND/mode_copilot",
                                        prefix + "/B748/ND/range_pilot",
                                        prefix + "/B748/ND/range_copilot"})
        .first->second;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FPS748FCUEfisProfile::buttonDefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    static std::unordered_map<bool, std::unordered_map<uint16_t, FCUEfisButtonDef>> cache;

    return cache.try_emplace(isSSG, std::unordered_map<uint16_t, FCUEfisButtonDef>{

                                        // MCP --------------------------------------------------------------------------
                                        // Buttons
                                        {0, {"SPD", prefix + "/UFMC/AP_SPD_Button"}},
                                        {1, {"LOC", prefix + "/UFMC/AP_VORLOC_Button"}},
                                        // {2, HDG/TRK — not on the 747-8 MCP},
                                        {3, {"AP1", prefix + "/UFMC/AP_CMDA_Button"}},
                                        {4, {"AP2", prefix + "/UFMC/AP_CMDB_Button"}},
                                        {5, {"A/THR", prefix + "/UFMC/AP_ARM_AT_Switch"}},
                                        {6, {"LNAV", prefix + "/UFMC/AP_LNAV_Button"}},
                                        {7, {"V/S", prefix + "/UFMC/AP_VS_Button"}},
                                        {8, {"APP", prefix + "/UFMC/AP_APP_Button"}},

                                        // Rotary encoders - Speed
                                        {9, {"SPD DEC", prefix + "/UFMC/Speed_Down"}},
                                        {10, {"SPD INC", prefix + "/UFMC/Speed_UP"}},
                                        {11, {"SPD INTV", prefix + "/UFMC/AP_SPD_Intervention_Button"}},

                                        // Rotary encoders - Heading
                                        {13, {"HDG DEC", prefix + "/UFMC/HDG_Down"}},
                                        {14, {"HDG INC", prefix + "/UFMC/HDG_UP"}},
                                        {15, {"HDG SEL", prefix + "/UFMC/AP_HDG_Button"}},
                                        {16, {"HDG HOLD", prefix + "/UFMC/AP_HDGHOLD_Button"}},

                                        // Rotary encoders - Altitude
                                        {17, {"ALT DEC", prefix + "/UFMC/Alt_Down"}},
                                        {18, {"ALT INC", prefix + "/UFMC/Alt_UP"}},
                                        {19, {"ALT INTV", prefix + "/UFMC/AP_Altitude_Intervention_Button"}},
                                        {20, {"ALT PULL", prefix + "/UFMC/AP_ALTHOLD_Button"}},

                                        // Rotary encoders - Vertical Speed
                                        {21, {"VS DEC", prefix + "/UFMC/VS_Down"}},
                                        {22, {"VS INC", prefix + "/UFMC/VS_UP"}},
                                        {23, {"V/S", prefix + "/UFMC/AP_VS_Button"}},

                                        // EFIS CAPT --------------------------------------------------------------------
                                        // Buttons
                                        {32, {"L_FD", prefix + "/UFMC/FD_Pilot_SW"}},
                                        {33, {"AP DISC", prefix + "/UFMC/AP_Discon"}},

                                        // ND overlay options
                                        {35, {"L_WPT", prefix + "/B748/ND/show_waypoint_pilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {36, {"L_STA", prefix + "/B748/ND/show_VOR_pilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {37, {"L_NDB", prefix + "/B748/ND/show_NDB_pilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {38, {"L_ARPT", prefix + "/B748/ND/show_airport_pilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},

                                        // BARO
                                        {39, {"L_BARO PUSH", "FPS/PFD/baro_standard", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {40, {"L_BARO PULL", "FPS/PFD/baro_standard", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {41, {"L_BARO DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
                                        {42, {"L_BARO INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
                                        {43, {"L_inHg", "FPS/PFD/baro_type_sw", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {44, {"L_hPa", "FPS/PFD/baro_type_sw", FCUEfisDatarefType::SET_VALUE, 1}},

                                        // ND Mode selector (0=APP/LS, 1=VOR, 2=MAP/ARC, 3=PLN/PLAN)
                                        {45, {"L_MODE APP", prefix + "/B748/ND/mode_pilot", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {46, {"L_MODE VOR", prefix + "/B748/ND/mode_pilot", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {48, {"L_MODE MAP", prefix + "/B748/ND/mode_pilot", FCUEfisDatarefType::SET_VALUE, 2}},
                                        {49, {"L_MODE PLAN", prefix + "/B748/ND/mode_pilot", FCUEfisDatarefType::SET_VALUE, 3}},

                                        // ND Range selector (-3=0.5nm, 1=10nm, 2=20nm, 3=40nm ... 7=640nm)
                                        {50, {"L_RANGE 10", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {51, {"L_RANGE 20", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 2}},
                                        {52, {"L_RANGE 40", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 3}},
                                        {53, {"L_RANGE 80", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 4}},
                                        {54, {"L_RANGE 160", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 5}},
                                        {55, {"L_RANGE 320", prefix + "/B748/ND/range_pilot", FCUEfisDatarefType::SET_VALUE, 6}},

                                        // VOR/ADF selectors (0=VOR, -1=OFF, 1=ADF)
                                        {56, {"L_VORL VOR", prefix + "/B748/MCP/ap_vor_adf1", FCUEfisDatarefType::SET_VALUE, 0.0}},
                                        {57, {"L_VORL OFF", prefix + "/B748/MCP/ap_vor_adf1", FCUEfisDatarefType::SET_VALUE, -1.0}},
                                        {58, {"L_VORL ADF", prefix + "/B748/MCP/ap_vor_adf1", FCUEfisDatarefType::SET_VALUE, 1.0}},
                                        {59, {"L_VORR VOR", prefix + "/B748/MCP/ap_vor_adf2", FCUEfisDatarefType::SET_VALUE, 0.0}},
                                        {60, {"L_VORR OFF", prefix + "/B748/MCP/ap_vor_adf2", FCUEfisDatarefType::SET_VALUE, -1.0}},
                                        {61, {"L_VORR ADF", prefix + "/B748/MCP/ap_vor_adf2", FCUEfisDatarefType::SET_VALUE, 1.0}},

                                        // EFIS FO ----------------------------------------------------------------------
                                        // Buttons
                                        {64, {"R_FD", prefix + "/UFMC/FD_Copilot_SW"}},
                                        {65, {"AP DISC", prefix + "/UFMC/AP_Discon"}},

                                        // ND overlay options
                                        {67, {"R_WPT", prefix + "/B748/ND/show_waypoint_copilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {68, {"R_STA", prefix + "/B748/ND/show_VOR_copilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {69, {"R_NDB", prefix + "/B748/ND/show_NDB_copilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},
                                        {70, {"R_ARPT", prefix + "/B748/ND/show_airport_copilot_sw", FCUEfisDatarefType::TOGGLE_VALUE}},

                                        // BARO
                                        {71, {"R_BARO PUSH", "FPS/PFD/baro_standard2", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {72, {"R_BARO PULL", "FPS/PFD/baro_standard2", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {73, {"R_BARO DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
                                        {74, {"R_BARO INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
                                        {75, {"R_inHg", "FPS/PFD/baro_type_sw", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {76, {"R_hPa", "FPS/PFD/baro_type_sw", FCUEfisDatarefType::SET_VALUE, 1}},

                                        // ND Mode selector (0=APP/LS, 1=VOR, 2=MAP/ARC, 3=PLN/PLAN)
                                        {77, {"R_MODE APP", prefix + "/B748/ND/mode_copilot", FCUEfisDatarefType::SET_VALUE, 0}},
                                        {78, {"R_MODE VOR", prefix + "/B748/ND/mode_copilot", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {80, {"R_MODE MAP", prefix + "/B748/ND/mode_copilot", FCUEfisDatarefType::SET_VALUE, 2}},
                                        {81, {"R_MODE PLAN", prefix + "/B748/ND/mode_copilot", FCUEfisDatarefType::SET_VALUE, 3}},

                                        // ND Range selector (-3=0.5nm, 1=10nm, 2=20nm, 3=40nm ... 7=640nm)
                                        {82, {"R_RANGE 10", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 1}},
                                        {83, {"R_RANGE 20", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 2}},
                                        {84, {"R_RANGE 40", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 3}},
                                        {85, {"R_RANGE 80", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 4}},
                                        {86, {"R_RANGE 160", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 5}},
                                        {87, {"R_RANGE 320", prefix + "/B748/ND/range_copilot", FCUEfisDatarefType::SET_VALUE, 6}},

                                        // VOR/ADF selectors (0=VOR, -1=OFF, 1=ADF)
                                        {88, {"R_VORL VOR", prefix + "/B748/MCP/ap_CP_vor_adf1", FCUEfisDatarefType::SET_VALUE, 0.0}},
                                        {89, {"R_VORL OFF", prefix + "/B748/MCP/ap_CP_vor_adf1", FCUEfisDatarefType::SET_VALUE, -1.0}},
                                        {90, {"R_VORL ADF", prefix + "/B748/MCP/ap_CP_vor_adf1", FCUEfisDatarefType::SET_VALUE, 1.0}},
                                        {91, {"R_VORR VOR", prefix + "/B748/MCP/ap_CP_vor_adf2", FCUEfisDatarefType::SET_VALUE, 0.0}},
                                        {92, {"R_VORR OFF", prefix + "/B748/MCP/ap_CP_vor_adf2", FCUEfisDatarefType::SET_VALUE, -1.0}},
                                        {93, {"R_VORR ADF", prefix + "/B748/MCP/ap_CP_vor_adf2", FCUEfisDatarefType::SET_VALUE, 1.0}}})
        .first->second;
}

void FPS748FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    auto dm = Dataref::getInstance();

    data.displayEnabled = dm->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
    data.displayTest = false;

    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::LevelChangeHeader;

    // Speed
    bool speedBlank = dm->getCached<float>((prefix + "/B748/mcp/speed_is_blank").c_str()) > 0.5f;
    bool isMach = dm->getCached<float>((prefix + "/B748/systems/athr/MCPSPD_spdmach").c_str()) > 0.5f;
    data.spdMach = isMach;

    if (speedBlank) {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::SpeedMachHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::SpeedMachValue;
        data.speed = "---";
    } else {
        float speed = dm->getCached<float>((prefix + "/B748/MCP/mcp_ias_mach_act").c_str());
        if (speed > 0) {
            std::stringstream ss;
            if (isMach) {
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
    data.spdManaged = false;

    // Heading
    bool isTrk = dm->getCached<int>((prefix + "/B748/ND/tk_hdg_pfd").c_str()) != 0;
    data.headingHdg = !isTrk;
    data.headingTrk = isTrk;
    data.headingLat = true;

    float heading = dm->getCached<float>((prefix + "/B748/MCP/mcp_heading_bug_act").c_str());
    if (heading >= 0) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << (static_cast<int>(heading) % 360);
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }
    data.hdgManaged = false;

    // Altitude
    float altitude = dm->getCached<float>((prefix + "/B748/MCP/mcp_alt_target_act").c_str());
    if (altitude > 0) {
        int altInt = static_cast<int>(std::round(altitude / 100.0f) * 100);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }
    data.altManaged = false;

    // Vertical speed
    bool vsBlank = dm->getCached<float>((prefix + "/B748/mcp/vs_is_blank").c_str()) > 0.5f;
    float vs = dm->getCached<float>((prefix + "/B748/MCP/mcp_vs_target_act").c_str());

    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    data.fpaIndication = false;
    data.fpaComma = false;
    data.vsVerticalLine = !vsBlank;

    if (vsBlank) {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::VerticalSpeedFPAHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::VerticalSpeedFPAValue;
        data.verticalSpeed = "----";
        data.vsSign = true;
    } else {
        std::stringstream ss;
        int absVs = std::abs(static_cast<int>(std::round(vs)));
        ss << std::setfill('0') << std::setw(4) << absVs;
        data.verticalSpeed = ss.str();
        data.vsSign = (vs >= 0);
    }

    // EFIS baro
    bool baroIsHpa = dm->getCached<bool>("FPS/PFD/baro_type_sw");
    float captainBaroValue = dm->getCached<float>("FPS/PFD/baro_now");
    bool captainIsStd = dm->getCached<bool>("FPS/PFD/baro_standard");
    for (int i = 0; i < 2; i++) {
        bool isCaptain = (i == 0);
        float foBaroValue = dm->getCached<float>("FPS/PFD/baro_now2");
        bool foIsStd = dm->getCached<bool>("FPS/PFD/baro_standard2");
        // FO baro_now2 = 0 means not independently set — mirror captain
        float baroValue = isCaptain ? captainBaroValue : (foBaroValue > 0 ? foBaroValue : captainBaroValue);
        bool isStd = isCaptain ? captainIsStd : (foBaroValue > 0 ? foIsStd : captainIsStd);

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

void FPS748FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        dm->set<float>(button->dataref.c_str(), static_cast<float>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        int current = dm->get<int>(button->dataref.c_str());
        dm->set<int>(button->dataref.c_str(), current ? 0 : 1);
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;
        const char *baroActDataref = isCaptain ? "FPS/PFD/baro_act" : "FPS/PFD/baro_act2";
        int current = dm->get<int>(baroActDataref);
        dm->set<int>(baroActDataref, current + (button->value > 0 ? 1 : -1));
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

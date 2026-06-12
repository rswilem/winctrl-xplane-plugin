#include "ff777-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

FF777FCUEfisProfile::FF777FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lights/glareshield", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("1-sim/output/mcp/ok");
        uint8_t target = hasPower ? brightness * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);

        uint8_t screenBrightness = hasPower ? 200 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        uint8_t ledBrightness = 255;
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? ledBrightness : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? ledBrightness : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? ledBrightness : 0);

        product->forceStateSync();
    },
        this);

    // We abuse the GPU hatch dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/anim/hatchGPU", [product](bool gpuHatchOpen) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/output/mcp/ok", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/glareshield");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/mcpCaptAP", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/mcpFoAP", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/mcpAT", [this, product](bool armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, armed || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/mcpLOC", [this, product](bool armed) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, armed || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/mcpAPP", [this, product](bool armed) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, armed || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/mcpFdLSwitch/anim", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, on || isTestMode() ? 1 : 0);
    },
        this);
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/mcpFdRSwitch/anim", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, on || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cptCAUTION", [this, product](bool isCaution) {
        bool isWarning = Dataref::getInstance()->getCached<bool>("1-sim/ckpt/lampsGlow/cptWARNING");
        product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/foCAUTION", [this, product](bool isCaution) {
        bool isWarning = Dataref::getInstance()->getCached<bool>("1-sim/ckpt/lampsGlow/foWARNING");
        product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cptWARNING", [this, product](bool on) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/cptCAUTION");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/foWARNING", [this, product](bool on) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/foCAUTION");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/ckpt/indLightTestSwitch/anim", [this, product](int isTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpCaptAP");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpFoAP");
        //Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/autothrottle_arm");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpLOC");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpAPP");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpAT");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/mcpFdLSwitch/anim");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/mcpFdRSwitch/anim");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/cptWARNING");
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/foWARNING");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/cptHsiStdButton/anim", [this, product](float animValue) {
        AppState::getInstance()->executeAfterDebounced("cptStdChanged", 50, this, [this, product]() {
            isStdCaptain = !isStdCaptain;

            float baroValue = Dataref::getInstance()->get<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
            if (isStdCaptain && fabs(baroValue - 29.92f) > std::numeric_limits<float>::epsilon()) {
                isStdCaptain = false;
            }

            product->updateDisplays();
        });
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/foHsiStdButton/anim", [this, product](float animValue) {
        AppState::getInstance()->executeAfterDebounced("foStdChanged", 50, this, [this, product]() {
            isStdFirstOfficer = !isStdFirstOfficer;

            float baroValue = Dataref::getInstance()->get<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");
            if (isStdFirstOfficer && fabs(baroValue - 29.92f) > std::numeric_limits<float>::epsilon()) {
                isStdFirstOfficer = false;
            }

            product->updateDisplays();
        });
    },
        this);
}

bool FF777FCUEfisProfile::IsEligible() {
    // FF777 datarefs that don't exist on the FF767
    return Dataref::getInstance()->exists("1-sim/ckpt/mcpApLButton/anim") &&
           Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

const std::vector<std::string> &FF777FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        //"T7Avionics/mcp_power",
        "1-sim/output/mcp/ok",

        // MCP - Speed
        //"sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        //"sim/cockpit/autopilot/airspeed_is_mach",
        "1-sim/output/mcp/isMachTrg",
        "1-sim/output/mcp/isHdgTrg",
        //"777/autopilot/speed_mode", // SPD, FLCH, etc.
        "1-sim/output/mcp/spd",
        "1-sim/output/mcp/fma_spd_mode",

        // MCP - Heading
        //"sim/cockpit/autopilot/heading_mag",
        //"777/autopilot/heading_hold_active",
        "1-sim/output/mcp/hdg",
        "1-sim/output/mcp/fma_hdg_mode",

        // MCP - Altitude
        //"sim/cockpit/autopilot/altitude",
        //"777/autopilot/altitude_hold_active",
        "1-sim/output/mcp/alt",
        "1-sim/output/mcp/fma_alt_mode",

        // MCP - Vertical Speed
        //"sim/cockpit/autopilot/vertical_velocity",
        //"777/autopilot/vnav_active",
        "1-sim/output/mcp/vs",
        "1-sim/output/mcp/fma_vs_mode",

        // EFIS - Barometric settings
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        // "777/displays/captain_baro_unit", // 0=inHg, 1=hPa
        // "777/displays/fo_baro_unit",
        "1-sim/output/efis/capt/baro_mode", // 0=inHg, 1=hPa
        "1-sim/output/efis/fo/baro_mode",

        // ND Mode and Range
        // "777/displays/captain_nd_mode", // 0=APP, 1=VOR, 2=MAP, 3=PLAN
        // "777/displays/fo_nd_mode",
        // "777/displays/captain_nd_range",
        // "777/displays/fo_nd_range",
        "1-sim/efis/capt/nd_mode", // 0=APP, 1=VOR, 2=MAP, 3=PLAN
        "1-sim/efis/FO/nd_mode",
        "1-sim/efis/capt/nd_range",
        "1-sim/efis/fo/nd_range",

        "1-sim/ckpt/indLightTestSwitch/anim",
        "1-sim/output/mcp/isSpdOpen",
        "1-sim/output/mcp/isVsOpen",
        "1-sim/ckpt/lampsGlow/mcpLNAV",
        "1-sim/ckpt/lampsGlow/mcpVNAV",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FF777FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {

        {0, {"MACH", "1-sim/command/mcpIasMachButton_button"}},
        {1, {"LOC", "1-sim/command/mcpLocButton_button"}},
        {2, {"TRK", "1-sim/command/mcpHdgTrkButton_button"}}, // TODO: Also add 1-sim/command/mcpVsFpaButton_button somehow
        {3, {"AP1", "1-sim/command/mcpApLButton_button"}},
        {4, {"AP2", "1-sim/command/mcpApRButton_button"}},
        {5, {"A/THR", "1-sim/command/mcpAtButton_button"}}, // TODO: turn off? maar on moet deze linkse ref ook weer callen. 1-sim/comm/armBothAutothrottles
        {6, {"EXPED", "1-sim/command/mcpAltRotary_push"}},
        {7, {"METRIC", ""}},
        {8, {"APPR", "1-sim/command/mcpAppButton_button"}},

        // Rotary encoders - Speed
        {9, {"SPD DEC", "1-sim/command/mcpSpdRotary_rotary-"}},
        {10, {"SPD INC", "1-sim/command/mcpSpdRotary_rotary+"}},
        {11, {"SPD PUSH", "1-sim/command/mcpSpdRotary_push"}},
        {12, {"SPD PULL", ""}},

        // Rotary encoders - Heading
        {13, {"HDG DEC", "1-sim/command/mcpHdgRotary_rotary-"}},
        {14, {"HDG INC", "1-sim/command/mcpHdgRotary_rotary+"}},
        {15, {"HDG PUSH", "1-sim/command/mcpLnavButton_button"}},
        {16, {"HDG PULL", "1-sim/command/mcpHdgCelButton_button"}},

        // Rotary encoders - Altitude
        {17, {"ALT DEC", "1-sim/command/mcpAltRotary_rotary-"}},
        {18, {"ALT INC", "1-sim/command/mcpAltRotary_rotary+"}},
        {19, {"ALT PUSH", "1-sim/command/mcpVnavButton_button"}},
        {20, {"ALT PULL", "1-sim/command/mcpFlchButton_button"}},

        // Rotary encoders - Vertical Speed
        {21, {"VS DEC", "1-sim/command/mcpVsRotary_rotary-"}},
        {22, {"VS INC", "1-sim/command/mcpVsRotary_rotary+"}},
        {23, {"VS PUSH", "1-sim/command/mcpAltHoldButton_button"}},
        {24, {"VS PULL", "1-sim/command/mcpVsButton_button"}},

        {25, {"ALT 100", "1-sim/command/mcpAltModeSwitch_set_0"}},
        {26, {"ALT 1000", "1-sim/command/mcpAltModeSwitch_set_1"}},

        {32, {"L_FD", "1-sim/command/mcpFdLSwitch_trigger"}},
        {33, {"L_LS", ""}},

        // ND Options
        {34, {"L_CSTR", "1-sim/command/cptHsiDataButton_button"}},
        {35, {"L_WPT", "1-sim/command/cptHsiWptButton_button"}},
        {36, {"L_VOR.D", "1-sim/command/cptHsiStaButton_button"}},
        {37, {"L_NDB", ""}},
        {38, {"L_ARPT", "1-sim/command/cptHsiArptButton_button"}},

        // BARO
        {39, {"L_STD PUSH", "1-sim/command/cptHsiStdButton_button"}},
        {40, {"L_STD PULL", "1-sim/command/cptHsiStdButton_button"}},
        {41, {"L_PRESS DEC", "1-sim/command/cptHsiBaroRotary_rotary-"}},
        {42, {"L_PRESS INC", "1-sim/command/cptHsiBaroRotary_rotary+"}},
        {43, {"L_inHg", "1-sim/command/cptHsiBaroModeRotary_set_0"}},
        {44, {"L_hPa", "1-sim/command/cptHsiBaroModeRotary_set_1"}},

        // ND Mode selector
        {45, {"L_MODE LS", "1-sim/command/cptHsiModeSwitch_set_0"}},
        {46, {"L_MODE VOR", "1-sim/command/cptHsiModeSwitch_set_1"}},
        {47, {"L_MODE NAV", ""}},
        {48, {"L_MODE ARC", "1-sim/command/cptHsiModeSwitch_set_2"}},
        {49, {"L_MODE PLAN", "1-sim/command/cptHsiModeSwitch_set_3"}},

        // ND Range selector
        {50, {"L_RANGE 10", "1-sim/command/cptHsiRangeSwitch_set_0"}},
        {51, {"L_RANGE 20", "1-sim/command/cptHsiRangeSwitch_set_1"}},
        {52, {"L_RANGE 40", "1-sim/command/cptHsiRangeSwitch_set_2"}},
        {53, {"L_RANGE 80", "1-sim/command/cptHsiRangeSwitch_set_3"}},
        {54, {"L_RANGE 160", "1-sim/command/cptHsiRangeSwitch_set_4"}},
        {55, {"L_RANGE 320", "1-sim/command/cptHsiRangeSwitch_set_5"}},

        // VOR/ADF selectors
        {56, {"L_1 VOR", "custom_vor_switch:cptHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {57, {"L_1 OFF", "custom_vor_switch:cptHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {58, {"L_1 ADF", "custom_vor_switch:cptHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {59, {"L_2 VOR", "custom_vor_switch:cptHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {60, {"L_2 OFF", "custom_vor_switch:cptHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {61, {"L_2 ADF", "custom_vor_switch:cptHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},

        {64, {"R_FD", "1-sim/command/mcpFdRSwitch_trigger"}},
        {65, {"R_LS", ""}},

        // ND Options
        {66, {"R_CSTR", "1-sim/command/foHsiDataButton_button"}},
        {67, {"R_WPT", "1-sim/command/foHsiWptButton_button"}},
        {68, {"R_VOR.D", "1-sim/command/foHsiStaButton_button"}},
        {69, {"R_NDB", ""}},
        {70, {"R_ARPT", "1-sim/command/foHsiArptButton_button"}},

        // BARO
        {71, {"R_STD PUSH", "1-sim/command/foHsiStdButton_button"}},
        {72, {"R_STD PULL", "1-sim/command/foHsiStdButton_button"}},
        {73, {"R_PRESS DEC", "1-sim/command/foHsiBaroRotary_rotary-"}},
        {74, {"R_PRESS INC", "1-sim/command/foHsiBaroRotary_rotary+"}},
        {75, {"R_inHg", "1-sim/command/foHsiBaroModeRotary_set_0"}},
        {76, {"R_hPa", "1-sim/command/foHsiBaroModeRotary_set_1"}},

        // ND Mode selector
        {77, {"R_MODE LS", "1-sim/command/foHsiModeSwitch_set_0"}},
        {78, {"R_MODE VOR", "1-sim/command/foHsiModeSwitch_set_1"}},
        {79, {"R_MODE NAV", ""}},
        {80, {"R_MODE ARC", "1-sim/command/foHsiModeSwitch_set_2"}},
        {81, {"R_MODE PLAN", "1-sim/command/foHsiModeSwitch_set_3"}},

        // ND Range selector
        {82, {"R_RANGE 10", "1-sim/command/foHsiRangeSwitch_set_0"}},
        {83, {"R_RANGE 20", "1-sim/command/foHsiRangeSwitch_set_1"}},
        {84, {"R_RANGE 40", "1-sim/command/foHsiRangeSwitch_set_2"}},
        {85, {"R_RANGE 80", "1-sim/command/foHsiRangeSwitch_set_3"}},
        {86, {"R_RANGE 160", "1-sim/command/foHsiRangeSwitch_set_4"}},
        {87, {"R_RANGE 320", "1-sim/command/foHsiRangeSwitch_set_5"}},

        // VOR/ADF selectors
        {88, {"R_1 VOR", "custom_vor_switch:foHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {89, {"R_1 OFF", "custom_vor_switch:foHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {90, {"R_1 ADF", "custom_vor_switch:foHsiVorLSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
        {91, {"R_2 VOR", "custom_vor_switch:foHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {92, {"R_2 OFF", "custom_vor_switch:foHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {93, {"R_2 ADF", "custom_vor_switch:foHsiVorRSwitch", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, -1.0}},
    };
    return buttons;
}

void FF777FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("1-sim/output/mcp/ok");
    data.displayTest = isTestMode();

    data.displayEnabledWindowsFlag = FCUDisplayData::Window::All;
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::LevelChangeHeader;

    data.spdMach = datarefManager->getCached<bool>("1-sim/output/mcp/isMachTrg");
    float speed = datarefManager->getCached<float>("1-sim/output/mcp/spd");

    bool isSpdOpen = datarefManager->getCached<bool>("1-sim/output/mcp/isSpdOpen");
    if (isSpdOpen) {
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
    } else {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::SpeedMachHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::SpeedMachValue;
    }

    data.spdManaged = false;

    float heading = datarefManager->getCached<float>("1-sim/output/mcp/hdg");
    if (heading >= 0) {
        int hdgDisplay = static_cast<int>(heading) % 360;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }

    data.hdgManaged = datarefManager->getCached<bool>("1-sim/ckpt/lampsGlow/mcpLNAV");
    data.headingHdg = datarefManager->getCached<bool>("1-sim/output/mcp/isHdgTrg");
    data.headingTrk = !data.headingHdg;

    float altitude = datarefManager->getCached<float>("1-sim/output/mcp/alt");
    if (altitude >= 0) {
        int altInt = static_cast<int>(altitude);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    data.altManaged = datarefManager->getCached<bool>("1-sim/ckpt/lampsGlow/mcpVNAV");

    float vs = datarefManager->getCached<float>("1-sim/output/mcp/vs");

    data.vsMode = true;
    data.fpaMode = false;

    bool isVsOpen = datarefManager->getCached<bool>("1-sim/output/mcp/isVsOpen");
    if (isVsOpen) {
        std::stringstream ss;
        int vsInt = static_cast<int>(std::round(vs));
        int absVs = std::abs(vsInt);

        ss << std::setfill('0') << std::setw(4) << absVs;
        data.verticalSpeed = ss.str();

        data.vsSign = (vs >= 0);
        data.fpaComma = false;
        data.vsIndication = true;
        data.fpaIndication = false;
        data.vsVerticalLine = true;
    } else {
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAHeader;
        data.displayEnabledWindowsFlag &= ~FCUDisplayData::VerticalSpeedFPAValue;
    }

    data.headingLat = false;

    for (int i = 0; i < 2; i++) {
        bool isCaptain = i == 0;

        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "1-sim/ckpt/cptHsiBaroModeRotary/anim" : "1-sim/ckpt/foHsiBaroModeRotary/anim");
        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = false,
            .isStd = (isCaptain && isStdCaptain) || (!isCaptain && isStdFirstOfficer),
        };

        if (!value.isStd && baroValue > 0) {
            value.setBaro(baroValue, !isBaroHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void FF777FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->dataref.rfind("custom_vor_switch", 0) == 0) {
        std::string prefix = "custom_vor_switch:";
        std::string target = button->dataref.substr(prefix.size());

        // Reset to 1 via _button, then decrement via trigger to reach target
        Dataref::getInstance()->executeCommand(("1-sim/command/" + target + "_button").c_str());

        // VOR=-1, OFF=0, ADF=1 — each trigger decrements by 1 from 1
        int ticksToTarget = 1 - static_cast<int>(button->value);
        std::string command = "1-sim/command/" + target + "_trigger";
        for (int i = 0; i < ticksToTarget; i++) {
            datarefManager->executeCommand(command.c_str());
        }
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        int currentValue = datarefManager->get<int>(button->dataref.c_str());
        int newValue = currentValue ? 0 : 1;
        datarefManager->set<int>(button->dataref.c_str(), newValue);
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

bool FF777FCUEfisProfile::isTestMode() {
    return Dataref::getInstance()->get<int>("1-sim/ckpt/indLightTestSwitch/anim") == 2;
}

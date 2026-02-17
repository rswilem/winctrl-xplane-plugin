#include "ff767-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

FF767FCUEfisProfile::FF767FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("lights/glareshield1_rhe",
        [product](float brightness) {
            bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");

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
        });

    // We abuse the GPU dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/electrical/gpuAvailable",
        [product](bool gpuDispo) {
            Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/autopilot_has_power");
        });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power",
        [product](bool hasPower) {
            Dataref::getInstance()->executeChangedCallbacksForDataref("lights/glareshield1_rhe");
        });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/AP/lnavButton", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/AP/vnavButton", [this, product](bool engaged) {
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, engaged || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/atSwitcher", [this, product](bool armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, !armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/locButton", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/appButton", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/cmd_L_Button", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/cmd_C_Button", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/cmd_R_Button", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EXPED_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/1/map4", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/2/map4", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/1/map5", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/2/map5", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/1/map2", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/2/map2", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/1/map3", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/efis/ctrlPanel/2/map3", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/desengageLever", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISL_LS_GREEN, !armed || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/AP/desengageLever", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::EFISR_LS_GREEN, !armed || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/AP/fd1Switcher", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, !on || isTestMode() ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/AP/fd2Switcher", [this, product](bool on) {
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, !on || isTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cptCAUTION",
        [this, product](bool isCaution) {
            bool isWarning = Dataref::getInstance()->getCached<bool>("1-sim/ckpt/lampsGlow/cptWARNING");
            product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/foCAUTION",
        [this, product](bool isCaution) {
            bool isWarning = Dataref::getInstance()->getCached<bool>("1-sim/ckpt/lampsGlow/foWARNING");
            product->setLedBrightness(FCUEfisLed::EFISR_CSTR_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
            product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, isCaution || isWarning || isTestMode() ? 1 : 0);
        });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/cptWARNING",
        [this, product](bool on) {
            Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/warnings/annunciators/master_caution");
        });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lampsGlow/foWARNING",
        [this, product](bool on) {
            Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/warnings/annunciators/master_caution");
        });

    Dataref::getInstance()->monitorExistingDataref<int>("1-sim/testPanel/test1Button",
        [this, product](int isTest) {
            Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/autopilot_has_power");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/mcpCaptAP");

            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/lnavButton");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/vnavButton");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/eprButton");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/locButton");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/appButton");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/cmd_L_Button");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/cmd_C_Button");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/AP/cmd_R_Button");

            Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/warnings/annunciators/master_warning");
            //Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lampsGlow/foWARNING");
            Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/comm/AP/ap_disc"); //OK
        });

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/efis/isBaroStdL",
        [this, product](float animValue) {
            AppState::getInstance()->executeAfterDebounced("cptStdChanged", 50, [this, product]() {
                isStdCaptain = !isStdCaptain;

                float baroValue = Dataref::getInstance()->get<float>("1-sim/gauges/baroINHg1_left");
                if (isStdCaptain && fabs(baroValue - 29.92f) > std::numeric_limits<float>::epsilon()) {
                    isStdCaptain = false;
                }

                product->updateDisplays();
            });
        });

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/efis/isBaroStdR",
        [this, product](float animValue) {
            AppState::getInstance()->executeAfterDebounced("foStdChanged", 50, [this, product]() {
                isStdFirstOfficer = !isStdFirstOfficer;

                float baroValue = Dataref::getInstance()->get<float>("1-sim/gauges/baroINHg1_right");
                if (isStdFirstOfficer && fabs(baroValue - 29.92f) > std::numeric_limits<float>::epsilon()) {
                    isStdFirstOfficer = false;
                }

                product->updateDisplays();
            });
        });
}

FF767FCUEfisProfile::~FF767FCUEfisProfile() {
    Dataref::getInstance()->unbind("lights/glareshield1_rhe");
    Dataref::getInstance()->unbind("1-sim/electrical/gpuAvailable");
    Dataref::getInstance()->unbind("sim/cockpit2/autopilot/autopilot_has_power");

    Dataref::getInstance()->unbind("1-sim/AP/lnavButton");
    Dataref::getInstance()->unbind("1-sim/AP/vnavButton");
    Dataref::getInstance()->unbind("1-sim/AP/eprButton");
    Dataref::getInstance()->unbind("1-sim/AP/locButton");
    Dataref::getInstance()->unbind("1-sim/AP/appButton");
    Dataref::getInstance()->unbind("1-sim/AP/cmd_L_Button");
    Dataref::getInstance()->unbind("1-sim/AP/cmd_C_Button");
    Dataref::getInstance()->unbind("1-sim/AP/cmd_R_Button");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/1/map2");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/2/map2");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/1/map3");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/2/map3");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/1/map4");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/2/map4");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/1/map5");
    Dataref::getInstance()->unbind("1-sim/efis/ctrlPanel/2/map5");

    //Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/cptCAUTION");
    //Dataref::getInstance()->unbind("1-sim/ckpt/lampsGlow/foCAUTION");
    Dataref::getInstance()->unbind("sim/cockpit/warnings/annunciators/master_caution");
    Dataref::getInstance()->unbind("sim/cockpit/warnings/annunciators/master_warning");
}

bool FF767FCUEfisProfile::IsEligible() {
    return (Dataref::getInstance()->exists("1-sim/AP/cmd_C_Button") &&
            !(Dataref::getInstance()->exists("1-sim/output/mcp/ok")));
}

const std::vector<std::string> &FF767FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {

        // MCP - Power
        "sim/cockpit2/autopilot/autopilot_has_power",
        "1-sim/AP/desengageLever"

        // MCP - Speed
        "1-sim/AP/iasmach",
        //"777/autopilot/speed_mode", // SPD, FLCH, etc.
        "1-sim/AP/dig3/spdSetting",
        //"1-sim/output/mcp/fma_spd_mode",

        // MCP - Heading
        // "1-sim/output/mcp/isHdgTrg",
        "1-sim/AP/hdgConfButton",
        "1-sim/AP/hdgSetting",
        //"1-sim/output/mcp/fma_hdg_mode",

        // MCP - Altitude
        "1-sim/AP/dig5/altSetting",
        //"1-sim/output/mcp/fma_alt_mode",

        // MCP - Vertical Speed
        "1-sim/AP/vviSetting",
        //"1-sim/output/mcp/fma_vs_mode",

        // EFIS - Barometric settings
        "1-sim/gauges/baroINHG1_left",
        "1-sim/gauges/baroINHG1_right",
        "1-sim/gauges/baroHPa1_left",
        "1-sim/gauges/baroHPa1_right",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "1-sim/ckpt/cptHsiStdButton/anim"
        "1-sim/ckpt/foHsiStdButton/anim"

        "1-sim/efis/isBaroHpaL", // 0=inHg,1=hPa
        "1-sim/efis/isBaroHpaR",

        // ND Mode and Range
        "1-sim/efis/ctrlPanel/1/hsiModeRotary",
        "1-sim/efis/ctrlPanel/2/hsiModeRotary",
        "1-sim/ndpanel/1/hsiRangeRotary",
        "1-sim/ndpanel/2/hsiRangeRotary",

        "1-sim/testPanel/test1Button",

        // ND Display options
        "1-sim/ckpt/cptHsiWptButton/anim"
        "1-sim/ckpt/cptHsiStaButton/anim"
        "1-sim/ckpt/cptHsiDataButton/anim"
        "1-sim/ckpt/cptHsiArptButton/anim"
        "1-sim/ckpt/foHsiWptButton/anim"
        "1-sim/ckpt/foHsiStaButton/anim"
        "1-sim/ckpt/foHsiDataButton/anim"
        "1-sim/ckpt/foHsiArptButton/anim"

    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FF767FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {

        // MCP --------------------------------------------------------------------------
        // Buttons
        {0, {"SPD", "1-sim/command/AP/iasmach_button"}},
        {1, {"LOC", "1-sim/comm/AP/locButton"}},
        // {2, {"HDG/TRK", "1-sim/command/mcpHdgTrkButton_button"}},    // does not exist on B767
        {3, {"AP1", "1-sim/comm/AP/lnavButton"}},
        {4, {"AP2", "1-sim/comm/AP/vnavButton"}},
        {5, {"A/THR", "1-sim/command/AP/atSwitcher_trigger"}},
        {6, {"EXPED", "1-sim/command/AP/cmd_L_Button_button"}},
        // {7, {"VS/FPA",  "1-sim/command/mcpVsFpaButton_button"}},     // does not exist on B767
        {8, {"APP", "1-sim/comm/AP/appButton"}},

        // Rotary encoders - Speed
        {9, {"SPD DEC", "1-sim/comm/AP/spdDN"}},
        {10, {"SPD INC", "1-sim/comm/AP/spdUP"}},
        {11, {"SPD", "1-sim/command/AP/spdConfButton_button"}},
        //{12, },

        // Rotary encoders - Heading
        {13, {"HDG DEC", "1-sim/comm/AP/hdgDN"}},
        {14, {"HDG INC", "1-sim/comm/AP/hdgUP"}},
        {15, {"HDG PUSH", "1-sim/command/AP/hdgConfButton_button"}},
        {16, {"HDG HOLD", "1-sim/comm/AP/hdgHoldButton"}},

        // Rotary encoders - Altitude
        {17, {"ALT DEC", "1-sim/comm/AP/altDN"}},
        {18, {"ALT INC", "1-sim/comm/AP/altUP"}},
        // {19, {"ALT PUSH", "1-sim/command/mcpAltRotary_push"}},       // does not exist on B767
        {20, {"ALT HOLD", "1-sim/comm/AP/altHoldButton"}},

        // Rotary encoders - Vertical Speed
        {21, {"VS DEC", "1-sim/comm/AP/vviDN"}},
        {22, {"VS INC", "1-sim/comm/AP/vviUP"}},
        {23, {"V/S", "1-sim/comm/AP/vviButton"}},
        //{24, },

        // Altitude par 100/1000 does not exist on B767
        // {25, {"ALT 100",  "1-sim/command/mcpAltModeSwitch_set_0"}},
        // {26, {"ALT 1000", "1-sim/command/mcpAltModeSwitch_set_1"}},

        // EFIS CAPT --------------------------------------------------------------------
        // Buttons
        {32, {"L_FD", "1-sim/command/AP/fd1Switcher_trigger"}},
        {33, {"AP DISC", "1-sim/command/AP/desengageLever_button"}},

        // ND Options
        {34, {"L_DATA", "1-sim/ckpt/cptHsiDataButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {35, {"L_WPT", "1-sim/ckpt/cptHsiWptButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {36, {"L_STA", "1-sim/ckpt/cptHsiStaButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        //{37, },
        {38, {"L_ARPT", "1-sim/ckpt/cptHsiArptButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},

        // BARO
        {39, {"L_BARO PUSH", "1-sim/ckpt/cptHsiStdButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {40, {"L_BARO PULL", "1-sim/ckpt/cptHsiStdButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {41, {"L_BARO DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}}, // ← "custom" cf. ButtonPressed
        {42, {"L_BARO INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},  // ← value > 0 = increase

        // Inverseur Baro
        {43, {"L_inHg", "1-sim/ckpt/cptHsiBaroModeRotary/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "1-sim/ckpt/cptHsiBaroModeRotary/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // ND Mode selector
        {45, {"L_MODE APP", "1-sim/ckpt/cptHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0}},
        {46, {"L_MODE VOR", "1-sim/ckpt/cptHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1}},
        //{47, },
        {48, {"L_MODE MAP", "1-sim/ckpt/cptHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 2}},
        {49, {"L_MODE PLAN", "1-sim/ckpt/cptHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 3}},

        // ND Range selector
        {50, {"L_RANGE 10", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0}},
        {51, {"L_RANGE 20", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1}},
        {52, {"L_RANGE 40", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 2}},
        {53, {"L_RANGE 80", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 3}},
        {54, {"L_RANGE 160", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 4}},
        {55, {"L_RANGE 320", "1-sim/ckpt/cptHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 5}},

        // VOR/ADF selectors
        {56, {"L_VORL VOR", "1-sim/ckpt/cptHsiVorLSwitch/anim/anim,1-sim/ckpt/cptHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, -1.0}},
        {57, {"L_VORL OFF", "1-sim/ckpt/cptHsiVorLSwitch/anim/anim,1-sim/ckpt/cptHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {58, {"L_VORL ADF", "1-sim/ckpt/cptHsiVorLSwitch/anim/anim,1-sim/ckpt/cptHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {59, {"L_VORR VOR", "1-sim/ckpt/cptHsiVorRSwitch/anim/anim,1-sim/ckpt/cptHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, -1.0}},
        {60, {"L_VORR OFF", "1-sim/ckpt/cptHsiVorRSwitch/anim/anim,1-sim/ckpt/cptHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {61, {"L_VORR ADF", "1-sim/ckpt/cptHsiVorRSwitch/anim/anim,1-sim/ckpt/cptHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // EFIS FO ----------------------------------------------------------------------
        // Buttons
        {64, {"R_FD", "1-sim/command/AP/fd2Switcher_trigger"}},
        {65, {"AP DISC", "1-sim/command/AP/desengageLever_button"}},

        // ND Options Buttons
        {66, {"R_DATA", "1-sim/ckpt/foHsiDataButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {67, {"R_WPT", "1-sim/ckpt/foHsiWptButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {68, {"R_STA", "1-sim/ckpt/foHsiStaButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        //{69, },
        {70, {"R_ARPT", "1-sim/ckpt/foHsiArptButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},

        // BARO
        {71, {"L_BARO PUSH", "1-sim/ckpt/foHsiStdButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {72, {"L_BARO PULL", "1-sim/ckpt/foHsiStdButton/anim", FCUEfisDatarefType::PUSH_BUTTON, 0}},
        {73, {"R_BARO DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_BARO INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},

        // Inverseur Baro
        {75, {"R_inHg", "1-sim/ckpt/foHsiBaroModeRotary/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "1-sim/ckpt/foHsiBaroModeRotary/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},

        // ND Mode selector
        {77, {"R_MODE APP", "1-sim/ckpt/foHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0}},
        {78, {"R_MODE VOR", "1-sim/ckpt/foHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1}},
        //{79, },
        {80, {"R_MODE MAP", "1-sim/ckpt/foHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 2}},
        {81, {"R_MODE PLAN", "1-sim/ckpt/foHsiModeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 3}},

        // ND Range selector
        {82, {"R_RANGE 10", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0}},
        {83, {"R_RANGE 20", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1}},
        {84, {"R_RANGE 40", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 2}},
        {85, {"R_RANGE 80", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 3}},
        {86, {"R_RANGE 160", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 4}},
        {87, {"R_RANGE 320", "1-sim/ckpt/foHsiRangeSwitch/anim", FCUEfisDatarefType::SET_VALUE, 5}},

        // VOR/ADF selectors
        {88, {"R_VORL VOR", "1-sim/ckpt/foHsiVorLSwitch/anim/anim,1-sim/ckpt/foHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, -1.0}},
        {89, {"R_VORL OFF", "1-sim/ckpt/foHsiVorLSwitch/anim/anim,1-sim/ckpt/foHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {90, {"R_VORL ADF", "1-sim/ckpt/foHsiVorLSwitch/anim/anim,1-sim/ckpt/foHsiVorLSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {91, {"R_VORR VOR", "1-sim/ckpt/foHsiVorRSwitch/anim/anim,1-sim/ckpt/foHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, -1.0}},
        {92, {"R_VORR OFF", "1-sim/ckpt/foHsiVorRSwitch/anim/anim,1-sim/ckpt/foHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {93, {"R_VORR ADF", "1-sim/ckpt/foHsiVorRSwitch/anim/anim,1-sim/ckpt/foHsiVorRSwitch/anim", FCUEfisDatarefType::SET_VALUE, 1.0}},

    };
    return buttons;
}

void FF767FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayTest = isTestMode();

    data.spdMach = datarefManager->getCached<bool>("1-sim/AP/iasmach");
    float speed = datarefManager->getCached<float>("1-sim/AP/dig3/spdSetting");

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

    data.spdManaged = false;

    // HDG ------------------------------------------------------------------------------
    float heading = datarefManager->getCached<float>("1-sim/AP/hdgSetting");
    if (heading >= 0) {
        int hdgDisplay = static_cast<int>(heading) % 360;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }

    data.hdgManaged = false;

    // to be disabled on 767: no TRK mode!?
    //data.hdgTrk = datarefManager->getCached<bool>("1-sim/AP/hdgConfButton") == false;

    // ALT ------------------------------------------------------------------------------
    float altitude = datarefManager->getCached<float>("1-sim/AP/dig5/altSetting");
    if (altitude > 0) {
        //int altInt = static_cast<int>(altitude);
        int altInt = static_cast<int>(std::round(altitude / 100.0f) * 100);
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    data.altManaged = false;

    float vs = datarefManager->getCached<float>("1-sim/AP/vviSetting");

    data.vsMode = true;
    data.fpaMode = false;

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

    data.latMode = true;

    for (int i = 0; i < 2; i++) {
        bool isCaptain = i == 0;

        bool isBaroHpa = datarefManager->getCached<bool>(isCaptain ? "1-sim/efis/isBaroHpaL" : "1-sim/efis/isBaroHpaR");

        bool isStdCapt = datarefManager->getCached<bool>("1-sim/efis/isBaroStdL");
        bool isStdFoff = datarefManager->getCached<bool>("1-sim/efis/isBaroStdR");

        float baroValue = datarefManager->getCached<float>(isCaptain ? "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot" : "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = data.displayTest,
            .baro = "",
            .unitIsInHg = false,
            //.isStd = (isCaptain && isStdCaptain) || (!isCaptain && isStdFirstOfficer), // MCN
            .isStd = (isCaptain && isStdCapt) || (!isCaptain && isStdFoff), // MCN
        };

        /*if (!value.isStd && baroValue > 0) {
            value.setBaro(baroValue, !isBaroHpa);
        } */

        if (value.isStd) {
            value.baro = "STD";
            value.unitIsInHg = false;
        } else if (baroValue > 0) {
            value.setBaro(baroValue, !isBaroHpa);
        }

        if (isCaptain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }
}

void FF767FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == FCUEfisDatarefType::PUSH_BUTTON) {
        datarefManager->set<float>(button->dataref.c_str(), phase == xplm_CommandBegin ? 1.0f : 0.0f);
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isCaptain = button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT;

        // Datarefs
        //const char *stdDataref = isCaptain ? "1-sim/efis/isBaroStdL" : "1-sim/efis/isBaroStdR";
        const char *animDataref = isCaptain ? "1-sim/ckpt/cptHsiBaroRotary/anim" : "1-sim/ckpt/foHsiBaroRotary/anim";

        float currentAnim = datarefManager->get<float>(animDataref);
        bool increase = button->value > 0;

        // Uniform increment/decrement: 1 unit = 0.01 inHg = 0.34 hPa
        float step = 1.0f;

        float newAnim = currentAnim + (increase ? step : -step);

        datarefManager->set<float>(animDataref, newAnim);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        // Split dataref by comma and set each one
        std::string datarefStr = button->dataref;
        size_t pos = 0;
        while ((pos = datarefStr.find(',')) != std::string::npos) {
            std::string dataref = datarefStr.substr(0, pos);
            datarefManager->set<float>(dataref.c_str(), button->value);
            datarefStr.erase(0, pos + 1);
        }

        // Set the last dataref
        if (!datarefStr.empty()) {
            datarefManager->set<float>(datarefStr.c_str(), button->value);
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        int currentValue = datarefManager->get<int>(button->dataref.c_str());
        int newValue = currentValue ? 0 : 1;
        datarefManager->set<int>(button->dataref.c_str(), newValue);
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

bool FF767FCUEfisProfile::isTestMode() {
    return Dataref::getInstance()->get<int>("1-sim/testPanel/test1Button") == 2;
}

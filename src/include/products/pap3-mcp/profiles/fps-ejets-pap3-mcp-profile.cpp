#include "fps-ejets-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <XPLMUtilities.h>

namespace {
    int normalizeDegrees(float value) {
        int degrees = static_cast<int>(std::lround(value));
        return (degrees % 360 + 360) % 360;
    }
}

FPSEjetsPAP3MCPProfile::FPSEjetsPAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    auto *dm = Dataref::getInstance();

    dm->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 11) {
            return;
        }
        bool hasPower = Dataref::getInstance()->getCached<float>("ssg/Elec/ac_alive") > 0.5f;
        uint8_t backlight = hasPower ? static_cast<uint8_t>(std::clamp(brightness[11], 0.0f, 1.0f) * 255) : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, backlight);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, hasPower ? 180 : 0);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, hasPower ? 180 : 0);
        product->forceStateSync();
    },
        this);

    dm->monitorExistingDataref<float>("ssg/Elec/ac_alive", [](float) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    },
        this);

    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_vnav_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VNAV, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_level_change_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("sim/cockpit2/autopilot/st55_hdg", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_lnav_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::LNAV, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_vor_loc_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_app_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::APP, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_alt_hold_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, v > 0.5f ? 1 : 0);
    },
        this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_vs_ann", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VS, v > 0.5f ? 1 : 0);
    },
        this);

    auto updateCommandLeds = [product](float) {
        auto *dr = Dataref::getInstance();
        bool engaged = dr->getCached<float>("ssg/B748/MCP/mcp_a_comm_ann") > 0.5f || dr->getCached<float>("ssg/B748/MCP/mcp_b_comm_ann") > 0.5f;
        product->setLedBrightness(PAP3MCPLed::CMD_A, engaged ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::CMD_B, engaged ? 1 : 0);
    };
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_a_comm_ann", updateCommandLeds, this);
    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_b_comm_ann", updateCommandLeds, this);

    dm->monitorExistingDataref<float>("ssg/B748/MCP/mcp_at_arm_act", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, v > 0.5f ? 1 : 0);
        product->setATSolenoid(v > 0.5f);
    },
        this);

    dm->executeChangedCallbacksForDataref("ssg/Elec/ac_alive");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_vnav_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_level_change_ann");
    dm->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/st55_hdg");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_lnav_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_vor_loc_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_app_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_alt_hold_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_vs_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_a_comm_ann");
    dm->executeChangedCallbacksForDataref("ssg/B748/MCP/mcp_at_arm_act");
}

bool FPSEjetsPAP3MCPProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

const std::vector<std::string> &FPSEjetsPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "ssg/Elec/ac_alive",
        "ssg/B748/MCP/mcp_ias_mach_act",
        "ssg/B748/MCP/mcp_ias_co_act",
        "ssg/B748/mcp/speed_is_blank",
        "ssg/B748/MCP/mcp_heading_bug_act",
        "ssg/EJET/MCP/bug_sel_alt",
        "ssg/B748/MCP/mcp_vs_target_act",
        "ssg/B748/mcp/vs_is_blank",
        "ssg/EJET/HSI/obs_bug_sel_pilot",
        "ssg/EJET/HSI/obs_bug_sel_copilot",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &FPSEjetsPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {2, {"VNAV", "FPS/EJET/MCP/VNAV_COMM"}},
        {3, {"FLCH", "FPS/EJET/MCP/FLCH_COMM"}},
        {4, {"HDG SEL", "FPS/EJET/MCP/HDG_COMM"}},
        {5, {"LNAV", "FPS/EJET/MCP/LNAV_COMM"}},
        {6, {"VOR LOC", "FPS/EJET/MCP/LNAV_COMM"}},
        {7, {"APP", "FPS/EJET/MCP/APP_COMM"}},
        {8, {"ALT HLD", "FPS/EJET/MCP/ALT_COMM"}},
        {9, {"V/S", "FPS/EJET/MCP/VS_COMM"}},
        {10, {"CMD A", "FPS/EJET/MCP/AP_COMM"}},
        {12, {"CMD B", "FPS/EJET/MCP/AP_COMM"}},
        {14, {"C/O", "ssg/B748/MCP/mcp_ias_co_act", PAP3MCPDatarefType::TOGGLE_VALUE}},
        {15, {"SPD INTV", "FPS/EJET/MCP/Speed_A_M"}},

        {17, {"CRS CAPT DEC", "FPS/EJET/MCP/CRS_P_Down"}},
        {18, {"CRS CAPT INC", "FPS/EJET/MCP/CRS_P_UP"}},
        {19, {"SPD DEC", "FPS/EJET/MCP/Speed_Down"}},
        {20, {"SPD INC", "FPS/EJET/MCP/Speed_UP"}},
        {21, {"HDG DEC", "FPS/EJET/MCP/HDG_Down"}},
        {22, {"HDG INC", "FPS/EJET/MCP/HDG_UP"}},
        {23, {"ALT DEC", "FPS/EJET/MCP/Alt_Down"}},
        {24, {"ALT INC", "FPS/EJET/MCP/Alt_UP"}},
        {25, {"CRS FO DEC", "FPS/EJET/MCP/CRS_CP_Down"}},
        {26, {"CRS FO INC", "FPS/EJET/MCP/CRS_CP_UP"}},
        {38, {"VS DEC", "FPS/EJET/MCP/VS_Down"}},
        {39, {"VS INC", "FPS/EJET/MCP/VS_UP"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &FPSEjetsPAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "FPS/EJET/MCP/CRS_P_UP", "FPS/EJET/MCP/CRS_P_Down"},
        {1, "SPD", "FPS/EJET/MCP/Speed_UP", "FPS/EJET/MCP/Speed_Down"},
        {2, "HDG", "FPS/EJET/MCP/HDG_UP", "FPS/EJET/MCP/HDG_Down"},
        {3, "ALT", "FPS/EJET/MCP/Alt_UP", "FPS/EJET/MCP/Alt_Down"},
        {4, "V/S", "FPS/EJET/MCP/VS_UP", "FPS/EJET/MCP/VS_Down"},
        {5, "CRS FO", "FPS/EJET/MCP/CRS_CP_UP", "FPS/EJET/MCP/CRS_CP_Down"},
    };
    return encoders;
}

void FPSEjetsPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto *dr = Dataref::getInstance();

    data.displayEnabled = dr->getCached<float>("ssg/Elec/ac_alive") > 0.5f;
    data.displayTest = false;
    data.showLabels = true;
    data.showCourse = true;

    data.spdMach = dr->getCached<float>("ssg/B748/MCP/mcp_ias_co_act") > 0.5f;
    data.machDigits = 2;
    data.speed = dr->getCached<float>("ssg/B748/MCP/mcp_ias_mach_act");
    data.speedVisible = dr->getCached<float>("ssg/B748/mcp/speed_is_blank") < 0.5f;

    data.heading = normalizeDegrees(dr->getCached<float>("ssg/B748/MCP/mcp_heading_bug_act"));
    data.headingVisible = true;

    data.altitude = std::max(0, static_cast<int>(std::lround(dr->getCached<float>("ssg/EJET/MCP/bug_sel_alt"))) * 100);

    data.verticalSpeed = dr->getCached<float>("ssg/B748/MCP/mcp_vs_target_act");
    data.verticalSpeedVisible = dr->getCached<float>("ssg/B748/mcp/vs_is_blank") < 0.5f;

    data.crsCapt = normalizeDegrees(dr->getCached<float>("ssg/EJET/HSI/obs_bug_sel_pilot"));
    data.crsFo = normalizeDegrees(dr->getCached<float>("ssg/EJET/HSI/obs_bug_sel_copilot"));
}

void FPSEjetsPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto *dr = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::TOGGLE_VALUE) {
        bool current = dr->get<float>(button->dataref.c_str()) > 0.5f;
        dr->set<float>(button->dataref.c_str(), current ? 0.0f : 1.0f);
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        dr->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        dr->executeCommand(button->dataref.c_str(), phase);
    }
}

void FPSEjetsPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void FPSEjetsPAP3MCPProfile::maybeToggle(const char *stateDataref, bool hwState, const char *toggleCmd) {
    if (!Dataref::getInstance()->exists(stateDataref)) {
        return;
    }
    bool simState = Dataref::getInstance()->get<float>(stateDataref) > 0.5f;
    if (simState != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

void FPSEjetsPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    if (byteOffset == 0x04 && bitMask == 0x08) {
        maybeToggle("ssg/B748/MCP/mcp_plt_fd_act", state, "FPS/EJET/MCP/FD_P_COMM");
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x20) {
        maybeToggle("ssg/B748/MCP/mcp_cplt_fd_act", state, "FPS/EJET/MCP/FD_CP_COMM");
        return;
    }

    if (byteOffset == 0x06 && (bitMask == 0x01 || bitMask == 0x02)) {
        if (state) {
            bool armed = bitMask == 0x01;
            product->setATSolenoid(armed);
            maybeToggle("ssg/B748/MCP/mcp_at_arm_act", armed, "FPS/EJET/MCP/AT_COMM");
        }
        return;
    }
}

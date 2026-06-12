#include "fps748-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

FPS748PAP3MCPProfile::FPS748PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    Dataref::getInstance()->monitorExistingDataref<float>((altPrefix + "/LGT/glaresheld_sw").c_str(), [product, altPrefix](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
        uint8_t backlight = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, backlight);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, hasPower ? 180 : 0);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, hasPower ? 180 : 0);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>((altPrefix + "/Elec/bus_1_powered").c_str(), [altPrefix](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/LGT/glaresheld_sw").c_str());
    }, this);

    // MCP mode LEDs — use _ann (annunciator) datarefs, not _act
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_n1_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::N1, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_speed_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::SPEED, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_vnav_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VNAV, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_level_change_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>("sim/cockpit2/autopilot/st55_hdg", [product](float v) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_lnav_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::LNAV, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_vor_loc_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_app_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::APP, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_alt_hold_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_vs_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::VS, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_a_comm_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, v > 0.5f ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_CAPT, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_b_comm_ann").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::CMD_B, v > 0.5f ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_FO, v > 0.5f ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<float>((prefix + "/B748/MCP/mcp_at_arm_act").c_str(), [product](float v) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, v > 0.5f ? 1 : 0);
        product->setATSolenoid(v > 0.5f);
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/Elec/bus_1_powered").c_str());

    auto *dm = Dataref::getInstance();
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_n1_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_speed_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_vnav_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_level_change_ann").c_str());
    dm->executeChangedCallbacksForDataref("sim/cockpit2/autopilot/st55_hdg");
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_lnav_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_vor_loc_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_app_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_alt_hold_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_vs_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_a_comm_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_b_comm_ann").c_str());
    dm->executeChangedCallbacksForDataref((prefix + "/B748/MCP/mcp_at_arm_act").c_str());
}

bool FPS748PAP3MCPProfile::IsSSGVersion() {
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

bool FPS748PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("FPS/748/simtime") || Dataref::getInstance()->exists("SSG/748/simtime");
}

const std::vector<std::string> &FPS748PAP3MCPProfile::displayDatarefs() const {
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
                                        prefix + "/B748/MCP/mcp_alt_target_act",
                                        prefix + "/B748/MCP/mcp_vs_target_act",
                                        prefix + "/B748/mcp/vs_is_blank",
                                        prefix + "/B748/MCP/mcp_plt_course_act",
                                        prefix + "/B748/MCP/mcp_cplt_course_act"})
        .first->second;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &FPS748PAP3MCPProfile::buttonDefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    static std::unordered_map<bool, std::unordered_map<uint16_t, PAP3MCPButtonDef>> cache;

    return cache.try_emplace(isSSG, std::unordered_map<uint16_t, PAP3MCPButtonDef>{
                                        // Row 1
                                        {0, {"N1", prefix + "/B748/MCP/mcp_thr_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {1, {"SPEED", prefix + "/B748/MCP/mcp_speed_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {2, {"VNAV", prefix + "/B748/MCP/mcp_vnav_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {3, {"LVL CHG", prefix + "/B748/MCP/mcp_level_change_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {4, {"HDG SEL", "custom_hdg_sel", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {5, {"LNAV", prefix + "/B748/MCP/mcp_lnav_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {6, {"VOR LOC", prefix + "/B748/MCP/mcp_vor_loc_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {7, {"APP", prefix + "/B748/MCP/mcp_app_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},

                                        // Row 2
                                        {8, {"ALT HLD", prefix + "/B748/MCP/mcp_alt_hold_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {9, {"V/S", prefix + "/B748/MCP/mcp_vert_speed_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {10, {"CMD A", prefix + "/B748/MCP/mcp_a_cmd_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {12, {"CMD B", prefix + "/B748/MCP/mcp_b_cmd_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
                                        {14, {"C/O", prefix + "/B748/MCP/mcp_ias_co_sw", PAP3MCPDatarefType::TOGGLE_VALUE}},
                                        {15, {"SPD INTV", prefix + "/B748/MCP/mcp_speed_int_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},

                                        // Row 3
                                        {16, {"ALT INTV", prefix + "/B748/MCP/mcp_alt_int_sw", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},

                                        // Encoder directions (also exposed as discrete buttons)
                                        {17, {"CRS CAPT DEC", "sim/autopilot/nav1_course_down"}},
                                        {18, {"CRS CAPT INC", "sim/autopilot/nav1_course_up"}},
                                        {19, {"SPD DEC", prefix + "/UFMC/Speed_Down"}},
                                        {20, {"SPD INC", prefix + "/UFMC/Speed_UP"}},
                                        {21, {"HDG DEC", prefix + "/UFMC/HDG_Down"}},
                                        {22, {"HDG INC", prefix + "/UFMC/HDG_UP"}},
                                        {23, {"ALT DEC", prefix + "/UFMC/Alt_Down"}},
                                        {24, {"ALT INC", prefix + "/UFMC/Alt_UP"}},
                                        {25, {"CRS FO DEC", "sim/autopilot/nav2_course_down"}},
                                        {26, {"CRS FO INC", "sim/autopilot/nav2_course_up"}},
                                        {38, {"VS DEC", prefix + "/UFMC/VS_Down"}},
                                        {39, {"VS INC", prefix + "/UFMC/VS_UP"}}})
        .first->second;
}

const std::vector<PAP3MCPEncoderDef> &FPS748PAP3MCPProfile::encoderDefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    static std::unordered_map<bool, std::vector<PAP3MCPEncoderDef>> cache;

    return cache.try_emplace(isSSG, std::vector<PAP3MCPEncoderDef>{
                                        {0, "CRS CAPT", "sim/autopilot/nav1_course_up", "sim/autopilot/nav1_course_down"},
                                        {1, "SPD", prefix + "/UFMC/Speed_UP", prefix + "/UFMC/Speed_Down"},
                                        {2, "HDG", prefix + "/UFMC/HDG_UP", prefix + "/UFMC/HDG_Down"},
                                        {3, "ALT", prefix + "/UFMC/Alt_UP", prefix + "/UFMC/Alt_Down"},
                                        {4, "V/S", prefix + "/UFMC/VS_UP", prefix + "/UFMC/VS_Down"},
                                        {5, "CRS FO", "sim/autopilot/nav2_course_up", "sim/autopilot/nav2_course_down"}})
        .first->second;
}

void FPS748PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    auto dm = Dataref::getInstance();

    data.displayEnabled = dm->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
    data.displayTest = false;
    data.showLabels = true;

    bool isMach = dm->getCached<float>((prefix + "/B748/systems/athr/MCPSPD_spdmach").c_str()) > 0.5f;
    data.digitA = isMach;
    data.machDigits = 3;
    float rawSpeed = dm->getCached<float>((prefix + "/B748/MCP/mcp_ias_mach_act").c_str());
    data.speed = rawSpeed;
    data.speedVisible = dm->getCached<float>((prefix + "/B748/mcp/speed_is_blank").c_str()) < 0.5f;

    data.heading = static_cast<int>(dm->getCached<float>((prefix + "/B748/MCP/mcp_heading_bug_act").c_str()));
    data.headingVisible = true;

    data.altitude = static_cast<int>(dm->getCached<float>((prefix + "/B748/MCP/mcp_alt_target_act").c_str()));

    data.verticalSpeed = dm->getCached<float>((prefix + "/B748/MCP/mcp_vs_target_act").c_str());
    data.verticalSpeedVisible = dm->getCached<float>((prefix + "/B748/mcp/vs_is_blank").c_str()) < 0.5f;

    data.crsCapt = static_cast<int>(dm->getCached<float>((prefix + "/B748/MCP/mcp_plt_course_act").c_str()));
    data.crsFo = static_cast<int>(dm->getCached<float>((prefix + "/B748/MCP/mcp_cplt_course_act").c_str()));
    data.showCourse = false;
}

void FPS748PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->dataref == "custom_hdg_sel") {
        std::string prefix = IsSSGVersion() ? "SSG" : "FPS";
        auto dm = Dataref::getInstance();
        if (phase == xplm_CommandBegin) {
            bool st55On = dm->get<float>("sim/cockpit2/autopilot/st55_hdg") > 0.5f;
            if (st55On) {
                dm->set<int>((prefix + "/B748/MCP/mcp_hdg_hold_sw").c_str(), 1);
            } else {
                dm->set<int>((prefix + "/B748/MCP/mcp_hdg_select_sw").c_str(), 1);
            }
        } else if (phase == xplm_CommandEnd) {
            dm->set<int>((prefix + "/B748/MCP/mcp_hdg_hold_sw").c_str(), 0);
            dm->set<int>((prefix + "/B748/MCP/mcp_hdg_select_sw").c_str(), 0);
        }
        return;
    }

    if (button->datarefType == PAP3MCPDatarefType::SET_VALUE_PHASED) {
        Dataref::getInstance()->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? static_cast<int>(button->value) : 0);
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::TOGGLE_VALUE) {
        int current = Dataref::getInstance()->get<int>(button->dataref.c_str());
        Dataref::getInstance()->set<int>(button->dataref.c_str(), current ? 0 : 1);
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void FPS748PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void FPS748PAP3MCPProfile::maybeToggle(const char *stateDataref, bool hwState, const char *toggleCmd) {
    if (!Dataref::getInstance()->exists(stateDataref)) {
        return;
    }
    bool simState = Dataref::getInstance()->get<bool>(stateDataref);
    if (simState != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

void FPS748PAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";

    // Hardware has 5 positions (no AUTO), mapped to values 1-5
    // 0x02 = pos 1 (smallest), 0x04 = pos 2, 0x08 = pos 3, 0x10 = pos 4, 0x20 = pos 5 (max)
    int target = -1;
    if (switchByte & 0x02) {
        target = 1;
    } else if (switchByte & 0x04) {
        target = 2;
    } else if (switchByte & 0x08) {
        target = 3;
    } else if (switchByte & 0x10) {
        target = 4;
    } else if (switchByte & 0x20) {
        target = 5;
    }

    if (target >= 1) {
        Dataref::getInstance()->set<float>((prefix + "/B748/MCP/mcp_bank_angle_act").c_str(), static_cast<float>(target));
    }
}

void FPS748PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";

    // FD CAPT: byte 0x04, bit 0x08
    if (byteOffset == 0x04 && bitMask == 0x08) {
        hwFDCaptOn = state;
        maybeToggle((prefix + "/B748/MCP/mcp_plt_fd_act").c_str(), hwFDCaptOn, (prefix + "/UFMC/FD_Pilot_SW").c_str());
        return;
    }

    // FD FO: byte 0x04, bit 0x20
    if (byteOffset == 0x04 && bitMask == 0x20) {
        hwFDFoOn = state;
        maybeToggle((prefix + "/B748/MCP/mcp_cplt_fd_act").c_str(), hwFDFoOn, (prefix + "/UFMC/FD_Copilot_SW").c_str());
        return;
    }

    // A/T ARM: bit 0x01 = armed, bit 0x02 = disarmed
    if (byteOffset == 0x06 && bitMask == 0x01) {
        if (state) {
            hwATOn = true;
            product->setATSolenoid(true);
            maybeToggle((prefix + "/B748/MCP/mcp_at_arm_act").c_str(), hwATOn, (prefix + "/UFMC/AP_ARM_AT_Switch").c_str());
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x02) {
        if (state) {
            hwATOn = false;
            product->setATSolenoid(false);
            maybeToggle((prefix + "/B748/MCP/mcp_at_arm_act").c_str(), hwATOn, (prefix + "/UFMC/AP_ARM_AT_Switch").c_str());
        }
        return;
    }

    // AP DISC: byte 0x04 bit 0x80 = bar down, byte 0x05 bit 0x01 = bar up
    // mcp_ap_disengage_act: 1 = disengaged (bar up/neutral), 0 = engaged/pressed (bar down)
    if (byteOffset == 0x04 && bitMask == 0x80) {
        if (state) {
            hwApDiscEngaged = false;
            Dataref::getInstance()->set<float>((prefix + "/B748/MCP/mcp_ap_disengage_act").c_str(), 0.0f);
        }
        return;
    }
    if (byteOffset == 0x05 && bitMask == 0x01) {
        if (state) {
            hwApDiscEngaged = true;
            Dataref::getInstance()->set<float>((prefix + "/B748/MCP/mcp_ap_disengage_act").c_str(), 1.0f);
        }
        return;
    }
}

#include "zibo-pap3-mcp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

ZiboPAP3MCPProfile::ZiboPAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [this, product](std::vector<float> panelBrightness) {
        if (panelBrightness.size() < 1) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        bool hasMainBus = Dataref::getInstance()->get<bool>("laminar/B738/electric/main_bus");
        float ratio = std::clamp(hasMainBus ? panelBrightness[0] : 0.5f, 0.0f, 1.0f);
        uint8_t brightness = hasPower ? ratio * 255 : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, brightness);

        uint8_t ledBrightness = hasPower && hasMainBus ? 180 : 0;
        if (isDisplayTestMode()) {
            ledBrightness = 255;
        }
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, ledBrightness);

        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");

        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, hasPower ? 180 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/dspl_light_test", [this](std::vector<float> displayTest) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");

        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/n1_status1");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/speed_status1");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/vnav_status1");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/lvl_chg_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/hdg_sel_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/lnav_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/vorloc_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/app_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/alt_hld_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/vs_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/cmd_a_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/cws_a_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/cmd_b_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/cws_b_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/autothrottle_status1");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/master_capt_status");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/autopilot/master_fo_status");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/electric/main_bus", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/n1_status1", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::N1, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/speed_status1", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::SPEED, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/vnav_status1", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::VNAV, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/lvl_chg_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/hdg_sel_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/lnav_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::LNAV, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/vorloc_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/app_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::APP, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/alt_hld_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/vs_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::VS, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/cmd_a_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/cws_a_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::CWS_A, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/cmd_b_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::CMD_B, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/cws_b_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::CWS_B, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/autothrottle_status1", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/master_capt_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::MA_CAPT, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/autopilot/master_fo_status", [this, product](float status) {
        product->setLedBrightness(PAP3MCPLed::MA_FO, status > 0.5f || isDisplayTestMode() ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/autopilot/autothrottle_arm_pos", [product](bool armed) {
        product->setATSolenoid(armed);
    });
}

ZiboPAP3MCPProfile::~ZiboPAP3MCPProfile() {
    Dataref::getInstance()->unbind("laminar/B738/electric/instrument_brightness");
    Dataref::getInstance()->unbind("laminar/B738/electric/panel_brightness");
    Dataref::getInstance()->unbind("laminar/B738/electric/main_bus");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("laminar/B738/dspl_light_test");

    Dataref::getInstance()->unbind("laminar/B738/autopilot/n1_status1");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/speed_status1");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/vnav_status1");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/lvl_chg_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/hdg_sel_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/lnav_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/vorloc_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/app_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/alt_hld_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/vs_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/cmd_a_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/cws_a_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/cmd_b_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/cws_b_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/autothrottle_status1");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/autothrottle_arm_pos");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/master_capt_status");
    Dataref::getInstance()->unbind("laminar/B738/autopilot/master_fo_status");
}

bool ZiboPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/autopilot/mcp_speed_dial_kts_mach");
}

const std::vector<std::string> &ZiboPAP3MCPProfile::displayDatarefs() const {
    static std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/avionics_on",
        "laminar/B738/electric/panel_brightness",
        "laminar/B738/electric/main_bus",
        "laminar/B738/autopilot/mcp_speed_dial_kts_mach",
        "laminar/B738/autopilot/mcp_hdg_dial",
        "laminar/B738/autopilot/mcp_alt_dial",
        "sim/cockpit2/autopilot/vvi_dial_fpm",
        "laminar/B738/autopilot/vvi_dial_show",
        "laminar/B738/autopilot/show_ias",
        "laminar/B738/autopilot/course_pilot",
        "laminar/B738/autopilot/course_copilot",
        "laminar/B738/mcp/digit_A",
        "laminar/B738/mcp/digit_8",
        "laminar/B738/dspl_light_test",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &ZiboPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1 (byte 0x01)
        {0, {"N1", "laminar/B738/autopilot/n1_press"}},
        {1, {"SPEED", "laminar/B738/autopilot/speed_press"}},
        {2, {"VNAV", "laminar/B738/autopilot/vnav_press"}},
        {3, {"LVL CHG", "laminar/B738/autopilot/lvl_chg_press"}},
        {4, {"HDG SEL", "laminar/B738/autopilot/hdg_sel_press"}},
        {5, {"LNAV", "laminar/B738/autopilot/lnav_press"}},
        {6, {"VOR LOC", "laminar/B738/autopilot/vorloc_press"}},
        {7, {"APP", "laminar/B738/autopilot/app_press"}},

        // Row 2 (byte 0x02)
        {8, {"ALT HLD", "laminar/B738/autopilot/alt_hld_press"}},
        {9, {"V/S", "laminar/B738/autopilot/vs_press"}},
        {10, {"CMD A", "laminar/B738/autopilot/cmd_a_press"}},
        {11, {"CWS A", "laminar/B738/autopilot/cws_a_press"}},
        {12, {"CMD B", "laminar/B738/autopilot/cmd_b_press"}},
        {13, {"CWS B", "laminar/B738/autopilot/cws_b_press"}},
        {14, {"C/O", "laminar/B738/autopilot/change_over_press"}},
        {15, {"SPD INTV", "laminar/B738/autopilot/spd_interv"}},

        // Row 3 (byte 0x03)
        {16, {"ALT INTV", "laminar/B738/autopilot/alt_interv"}},

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", "laminar/B738/autopilot/course_pilot_dn"}},
        {18, {"CRS CAPT INC", "laminar/B738/autopilot/course_pilot_up"}},
        {19, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {20, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {21, {"HDG DEC", "laminar/B738/autopilot/heading_dn"}},
        {22, {"HDG INC", "laminar/B738/autopilot/heading_up"}},
        {23, {"ALT DEC", "laminar/B738/autopilot/altitude_dn"}},
        {24, {"ALT INC", "laminar/B738/autopilot/altitude_up"}},
        {25, {"CRS FO DEC", "laminar/B738/autopilot/course_copilot_dn"}},
        {26, {"CRS FO INC", "laminar/B738/autopilot/course_copilot_up"}},
        {38, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {39, {"VS INC", "sim/autopilot/vertical_speed_up"}}};
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &ZiboPAP3MCPProfile::encoderDefs() const {
    static std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "laminar/B738/autopilot/course_pilot_up", "laminar/B738/autopilot/course_pilot_dn"},
        {1, "SPD", "sim/autopilot/airspeed_up", "sim/autopilot/airspeed_down"},
        {2, "HDG", "laminar/B738/autopilot/heading_up", "laminar/B738/autopilot/heading_dn"},
        {3, "ALT", "laminar/B738/autopilot/altitude_up", "laminar/B738/autopilot/altitude_dn"},
        {4, "V/S", "sim/autopilot/vertical_speed_up", "sim/autopilot/vertical_speed_down"},
        {5, "CRS FO", "laminar/B738/autopilot/course_copilot_up", "laminar/B738/autopilot/course_copilot_dn"}};
    return encoders;
}

void ZiboPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();
    data.showLabels = false; // Zibo MCP does not have labels on the display
    data.speed = dataref->getCached<float>("laminar/B738/autopilot/mcp_speed_dial_kts_mach");
    data.heading = dataref->getCached<int>("laminar/B738/autopilot/mcp_hdg_dial");
    data.altitude = dataref->getCached<int>("laminar/B738/autopilot/mcp_alt_dial");
    data.verticalSpeed = dataref->getCached<float>("sim/cockpit2/autopilot/vvi_dial_fpm");
    data.verticalSpeedVisible = dataref->getCached<bool>("laminar/B738/autopilot/vvi_dial_show");
    data.speedVisible = dataref->getCached<bool>("laminar/B738/autopilot/show_ias");
    data.crsCapt = dataref->getCached<int>("laminar/B738/autopilot/course_pilot");
    data.crsFo = dataref->getCached<int>("laminar/B738/autopilot/course_copilot");

    // Special display digits
    data.digitA = dataref->getCached<bool>("laminar/B738/mcp/digit_A");
    data.digitB = dataref->getCached<bool>("laminar/B738/mcp/digit_8");

    // Display test mode (all segments lit when test mode is 1, no segments when 2)
    std::vector<float> displayTest = dataref->getCached<std::vector<float>>("laminar/B738/dspl_light_test");
    uint8_t displayTestMode = static_cast<uint8_t>(displayTest.size() > 0 ? displayTest[0] : 0.0f);
    data.displayTest = displayTestMode >= 1;
    data.displayEnabled = displayTestMode != 2 && dataref->getCached<bool>("sim/cockpit/electrical/avionics_on");
}

void ZiboPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

// Bank angle switch handling (5-position rotary switch)
int ZiboPAP3MCPProfile::readBankAngleIndex() {
    if (!Dataref::getInstance()->exists("laminar/B738/autopilot/bank_angle_pos")) {
        return -1;
    }
    return Dataref::getInstance()->get<int>("laminar/B738/autopilot/bank_angle_pos");
}

void ZiboPAP3MCPProfile::setBankAngleIndex(int target) {
    if (target < 0 || target > 4) {
        return;
    }

    int current = readBankAngleIndex();
    if (current < 0 || current == target) {
        return;
    }

    // X-Plane processes commands at end-of-frame, so the dataref won't update
    // within the same call. Calculate delta up front and fire all commands at once.
    const char *cmd = (target > current) ? "laminar/B738/autopilot/bank_angle_up" : "laminar/B738/autopilot/bank_angle_dn";
    int steps = std::abs(target - current);
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void ZiboPAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    // Bank angle switch uses bits in byte 0x05:
    // 0x02 = 10° (index 0)
    // 0x04 = 15° (index 1)
    // 0x08 = 20° (index 2)
    // 0x10 = 25° (index 3)
    // 0x20 = 30° (index 4)

    int target = -1;

    if (switchByte & 0x02) {
        target = 0; // 10°
    } else if (switchByte & 0x04) {
        target = 1; // 15°
    } else if (switchByte & 0x08) {
        target = 2; // 20°
    } else if (switchByte & 0x10) {
        target = 3; // 25°
    } else if (switchByte & 0x20) {
        target = 4; // 30°
    }

    if (target >= 0) {
        setBankAngleIndex(target);
    }
}

// Helper: Toggle sim state if it doesn't match hardware state
void ZiboPAP3MCPProfile::maybeToggle(const char *dataref, bool hwState, const char *toggleCmd) {
    if (!Dataref::getInstance()->exists(dataref)) {
        return;
    }

    bool simState = Dataref::getInstance()->get<bool>(dataref);
    if (simState != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

// Handle maintained switches (FD CAPT/FO, A/T, AP Disconnect)
void ZiboPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT: byte 0x04, bit 0x08 = OFF line; bit is HIGH when switch is OFF, LOW when switch is ON
    if (byteOffset == 0x04 && bitMask == 0x08) {
        hwFDCaptOn = !state; // OFF line active (state=true) means switch is OFF
        maybeToggle("laminar/B738/autopilot/flight_director_pos", hwFDCaptOn, "laminar/B738/autopilot/flight_director_toggle");
        return;
    }

    // FD FO: byte 0x04, bit 0x20 = OFF line; bit is HIGH when switch is OFF, LOW when switch is ON
    if (byteOffset == 0x04 && bitMask == 0x20) {
        hwFDFoOn = !state; // OFF line active (state=true) means switch is OFF
        maybeToggle("laminar/B738/autopilot/flight_director_fo_pos", hwFDFoOn, "laminar/B738/autopilot/flight_director_fo_toggle");
        return;
    }

    // A/T ARM: byte 0x06, bit 0x01 = ARMED line, bit 0x02 = DISARMED line
    if (byteOffset == 0x06 && bitMask == 0x01) {
        if (state) {
            hwATOn = true;
            product->setATSolenoid(true);
            if (Dataref::getInstance()->exists("laminar/B738/autopilot/autothrottle_arm_pos")) {
                maybeToggle("laminar/B738/autopilot/autothrottle_arm_pos", hwATOn, "laminar/B738/autopilot/autothrottle_arm_toggle");
            }
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x02) {
        if (state) {
            hwATOn = false;
            product->setATSolenoid(false);
            if (Dataref::getInstance()->exists("laminar/B738/autopilot/autothrottle_arm_pos")) {
                maybeToggle("laminar/B738/autopilot/autothrottle_arm_pos", hwATOn, "laminar/B738/autopilot/autothrottle_arm_toggle");
            }
        }
        return;
    }

    // AP DISCONNECT: byte 0x04, bit 0x80 = UP line, byte 0x05, bit 0x01 = DOWN line
    // The bar is a latching (non-spring) switch. When it moves, both lines transition
    // in the same frame. Only act on the rising edge of each line to avoid double-toggle.
    if (byteOffset == 0x04 && bitMask == 0x80) {
        if (state) {
            // UP line rising: bar just clicked back up (re-engage position)
            hwApDiscEngaged = true;
            maybeToggle("laminar/B738/autopilot/disconnect_pos", hwApDiscEngaged, "laminar/B738/autopilot/disconnect_toggle");
        }
        return;
    }
    if (byteOffset == 0x05 && bitMask == 0x01) {
        if (state) {
            // DOWN line rising: bar just clicked down (disengage position)
            hwApDiscEngaged = false;
            maybeToggle("laminar/B738/autopilot/disconnect_pos", hwApDiscEngaged, "laminar/B738/autopilot/disconnect_toggle");
        }
        return;
    }
}

bool ZiboPAP3MCPProfile::isDisplayTestMode() {
    std::vector<float> displayTest = Dataref::getInstance()->getCached<std::vector<float>>("laminar/B738/dspl_light_test");
    uint8_t displayTestMode = static_cast<uint8_t>(displayTest.size() > 0 ? displayTest[0] : 0.0f);
    return displayTestMode > 0;
}

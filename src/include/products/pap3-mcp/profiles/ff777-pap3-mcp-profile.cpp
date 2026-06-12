#include "ff777-pap3-mcp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

FF777PAP3MCPProfile::FF777PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    // Monitor power and brightness
    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lights/glareshield", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>("1-sim/output/mcp/ok");

        float ratio = std::clamp(brightness, 0.0f, 1.0f);
        uint8_t panelBrightness = hasPower ? static_cast<uint8_t>(ratio * 255) : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, panelBrightness);

        uint8_t lcdBrightness = hasPower ? 180 : 0;
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, lcdBrightness);

        uint8_t ledBrightness = hasPower ? std::max(static_cast<uint8_t>(ratio * 255), static_cast<uint8_t>(153)) : 0; // At least 0.6 brightness
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, ledBrightness);

        product->forceStateSync();
    }, this);

    // We abuse the GPU hatch dataref to trigger an update when the UI is closed.
    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/anim/hatchGPU", [product](bool gpuHatchOpen) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/output/mcp/ok");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/glareshield");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/output/mcp/ok", [product](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/glareshield");
    }, this);

    // Monitor LEDs - FlightFactor 777 lamp glow datarefs
    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpVNAV", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::VNAV, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpFLCH", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpLNAV", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::LNAV, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpLOC", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpAPP", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::APP, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpAltHOLD", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpVS", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::VS, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpCaptAP", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, status > 0.5f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lampsGlow/mcpAT", [product](float status) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, status > 0.5f ? 1 : 0);
    }, this);
}

bool FF777PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("1-sim/output/mcp/spd");
}

const std::vector<std::string> &FF777PAP3MCPProfile::displayDatarefs() const {
    static std::vector<std::string> datarefs = {
        "sim/cockpit2/autopilot/autopilot_has_power",
        "1-sim/ckpt/lights/glareshield",
        "1-sim/output/mcp/spd",
        "1-sim/output/mcp/hdg",
        "1-sim/output/mcp/alt",
        "1-sim/output/mcp/vs",
        "1-sim/output/mcp/isSpdOpen",
        "1-sim/output/mcp/isVsOpen",
        "sim/cockpit/radios/nav1_obs_degm",
        "sim/cockpit/radios/nav2_obs_degm",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &FF777PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1 (byte 0x01) - Main autopilot mode buttons
        {0, {"CLB/N1", "1-sim/command/mcpClbButton_button"}},        // CLB (THR REF) / N1 equivalent
        {1, {"SPEED", "1-sim/command/mcpAtButton_button"}},          // SPEED
        {2, {"VNAV", "1-sim/command/mcpVnavButton_button"}},         // VNAV
        {3, {"FLCH/LVL CHG", "1-sim/command/mcpFlchButton_button"}}, // FLCH (LVL CHG)
        {4, {"HDG SEL", "1-sim/command/mcpHdgCelButton_button"}},    // HDG SEL
        {5, {"LNAV", "1-sim/command/mcpLnavButton_button"}},         // LNAV
        {6, {"LOC/VORLOC", "1-sim/command/mcpLocButton_button"}},    // LOC (VORLOC)
        {7, {"APP", "1-sim/command/mcpAppButton_button"}},           // APP

        // Row 2 (byte 0x02) - Additional autopilot buttons
        {8, {"ALT HLD", "1-sim/command/mcpAltHoldButton_button"}}, // ALT HOLD
        {9, {"V/S", "1-sim/command/mcpVsButton_button"}},          // V/S
        {10, {"CMD A", "1-sim/command/mcpApLButton_button"}},      // CMD A (AP engage left)
        {11, {"CWS A", "1-sim/command/mcpApLButton_button"}},      // CWS A (map to CMD A - FF777 doesn't have separate CWS)
        {12, {"CMD B", "1-sim/command/mcpApRButton_button"}},      // CMD B (AP engage right)
        {13, {"CWS B", "1-sim/command/mcpApRButton_button"}},      // CWS B (map to CMD B)
        {14, {"C/O", "1-sim/command/mcpIasMachButton_button"}},    // CHANGE OVER (IAS/MACH toggle)
        {15, {"SPD INTV", "1-sim/command/mcpSpdRotary_push"}},     // SPD INTERV (SPD push)

        // Row 3 (byte 0x03)
        {16, {"ALT INTV", "1-sim/command/mcpAltRotary_push"}}, // ALT INTERV (ALT push)

        // Encoder rotations (exposed as button pairs by the hardware)
        {19, {"SPD DEC", "1-sim/command/mcpSpdRotary_rotary-"}},
        {20, {"SPD INC", "1-sim/command/mcpSpdRotary_rotary+"}},
        {21, {"HDG DEC", "1-sim/command/mcpHdgRotary_rotary-"}},
        {22, {"HDG INC", "1-sim/command/mcpHdgRotary_rotary+"}},
        {23, {"ALT DEC", "1-sim/command/mcpAltRotary_rotary-"}},
        {24, {"ALT INC", "1-sim/command/mcpAltRotary_rotary+"}},
        {38, {"VS DEC", "1-sim/command/mcpVsRotary_rotary-"}},
        {39, {"VS INC", "1-sim/command/mcpVsRotary_rotary+"}}}; // ALT INTERV (ALT push)
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &FF777PAP3MCPProfile::encoderDefs() const {
    static std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "", ""}, // 777 doesn't have course on MCP (it's on EFIS panel)
        {1, "SPD", "1-sim/command/mcpSpdRotary_rotary+", "1-sim/command/mcpSpdRotary_rotary-"},
        {2, "HDG", "1-sim/command/mcpHdgRotary_rotary+", "1-sim/command/mcpHdgRotary_rotary-"},
        {3, "ALT", "1-sim/command/mcpAltRotary_rotary+", "1-sim/command/mcpAltRotary_rotary-"},
        {4, "V/S", "1-sim/command/mcpVsRotary_rotary+", "1-sim/command/mcpVsRotary_rotary-"},
        {5, "CRS FO", "", ""}}; // 777 doesn't have course on MCP (it's on EFIS panel)
    return encoders;
}

void FF777PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();
    data.showLabels = true;              // FF777 MCP has labels on the display
    data.showDashesWhenInactive = false; // FF777 doesn't show dashes when inactive
    data.showLabelsWhenInactive = true;  // FF777 shows labels even when displays are inactive
    data.showCourse = false;             // 777 doesn't have course on MCP (it's on EFIS panel)
    bool hasPower = dataref->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayEnabled = hasPower;

    if (!hasPower) {
        return;
    }

    data.speed = dataref->getCached<float>("1-sim/output/mcp/spd");
    data.heading = dataref->getCached<int>("1-sim/output/mcp/hdg");
    data.altitude = dataref->getCached<int>("1-sim/output/mcp/alt");
    data.verticalSpeed = dataref->getCached<float>("1-sim/output/mcp/vs");
    data.speedVisible = dataref->getCached<bool>("1-sim/output/mcp/isSpdOpen");
    data.verticalSpeedVisible = dataref->getCached<bool>("1-sim/output/mcp/isVsOpen");

    // 777 doesn't have course on MCP (it's on EFIS panel), but we read standard X-Plane datarefs for reference
    data.crsCapt = dataref->getCached<int>("sim/cockpit/radios/nav1_obs_degm");
    data.crsFo = dataref->getCached<int>("sim/cockpit/radios/nav2_obs_degm");

    // FF777 doesn't have special digit flags
    data.digitA = false;
    data.digitB = false;
    data.displayTest = false;
}

void FF777PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void FF777PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    // Skip course encoders (777 doesn't have them on MCP)
    if (encoder->incCmd.empty() || encoder->decCmd.empty()) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

// Helper: Toggle sim state if it doesn't match hardware state
void FF777PAP3MCPProfile::maybeToggle(const char *posDataref, bool hwState, const char *toggleCmd) {
    if (!Dataref::getInstance()->exists(posDataref)) {
        return;
    }

    int simState = Dataref::getInstance()->get<int>(posDataref);
    bool simOn = (simState == 1); // FF777 uses 1 for ON, 0 for OFF

    if (simOn != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

// Handle maintained switches (FD Left/Right, A/T Left/Right, AP Disconnect)
void FF777PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT (Left): byte 0x04, bit 0x08 (inverted - pressed = OFF line, so pressed means switch is ON)
    if (byteOffset == 0x04 && bitMask == 0x08) {
        hwFDLeftOn = state;
        maybeToggle("1-sim/ckpt/mcpFdLSwitch/anim", hwFDLeftOn, "1-sim/command/mcpFdLSwitch_trigger");
        return;
    }

    // FD FO (Right): byte 0x04, bit 0x20 (inverted - pressed = OFF line, so pressed means switch is ON)
    if (byteOffset == 0x04 && bitMask == 0x20) {
        hwFDRightOn = state;
        maybeToggle("1-sim/ckpt/mcpFdRSwitch/anim", hwFDRightOn, "1-sim/command/mcpFdRSwitch_trigger");
        return;
    }

    // A/T ARM Left: byte 0x06, bit 0x01 = ARMED line, bit 0x02 = DISARMED line
    if (byteOffset == 0x06 && bitMask == 0x01) {
        if (state) {
            hwATLeftOn = true;
            maybeToggle("1-sim/ckpt/mcpAtSwitchL/anim", hwATLeftOn, "1-sim/command/mcpAtSwitchL_trigger");
            maybeToggle("1-sim/ckpt/mcpAtSwitchR/anim", hwATLeftOn, "1-sim/command/mcpAtSwitchR_trigger");
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x02) {
        if (state) {
            hwATLeftOn = false;
            maybeToggle("1-sim/ckpt/mcpAtSwitchL/anim", hwATLeftOn, "1-sim/command/mcpAtSwitchL_trigger");
            maybeToggle("1-sim/ckpt/mcpAtSwitchR/anim", hwATLeftOn, "1-sim/command/mcpAtSwitchR_trigger");
        }
        return;
    }

    // A/T ARM Right: Using right A/T switch for solenoid control
    if (byteOffset == 0x06 && bitMask == 0x04) {
        if (state) {
            hwATRightOn = true;
            product->setATSolenoid(true);
            maybeToggle("1-sim/ckpt/mcpAtSwitchR/anim", hwATRightOn, "1-sim/command/mcpAtSwitchR_trigger");
            maybeToggle("1-sim/ckpt/mcpAtSwitchL/anim", hwATRightOn, "1-sim/command/mcpAtSwitchL_trigger");
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x08) {
        if (state) {
            hwATRightOn = false;
            product->setATSolenoid(false);
            maybeToggle("1-sim/ckpt/mcpAtSwitchR/anim", hwATRightOn, "1-sim/command/mcpAtSwitchR_trigger");
            maybeToggle("1-sim/ckpt/mcpAtSwitchL/anim", hwATRightOn, "1-sim/command/mcpAtSwitchL_trigger");
        }
        return;
    }

    // AP DISCONNECT: byte 0x04, bit 0x80 = UP line (inverted), byte 0x05, bit 0x01 = DOWN line
    // Only handle the UP line transition to avoid double-toggling when switch moves through both lines
    if (byteOffset == 0x04 && bitMask == 0x80) {
        // UP line: pressed = engaged, released = disengaged
        bool newEngagedState = !state;
        if (newEngagedState != hwApDiscEngaged) {
            hwApDiscEngaged = newEngagedState;
            maybeToggle("1-sim/ckpt/mcpApDiscSwitch/anim", hwApDiscEngaged, "1-sim/command/mcpApDiscSwitch_trigger");
        }
        return;
    }
}

// Bank angle switch handler (5-position rotary switch)
// Maps hardware positions to FF777 bank angle indices:
// FF777 indices: 0=AUTO, 1=5°, 2=10°, 3=15°, 4=20°, 5=25°
// Hardware positions (5 positions): AUTO, 5°, 10°, 20°, 25° (skip 15°)
// Hardware bits: 0x02=pos1, 0x04=pos2, 0x08=pos3, 0x10=pos4, 0x20=pos5
void FF777PAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    int target = 0; // Default to AUTO

    if (switchByte & 0x02) {
        target = 0; // AUTO
    } else if (switchByte & 0x04) {
        target = 1; // 5°
    } else if (switchByte & 0x08) {
        target = 2; // 10°
    } else if (switchByte & 0x10) {
        target = 4; // 20° (skip 15°)
    } else if (switchByte & 0x20) {
        target = 5; // 25°
    }

    // Build the command name and execute
    std::string cmd = "1-sim/command/mcpBankAngleSwitch_set_" + std::to_string(target);
    Dataref::getInstance()->executeCommand(cmd.c_str());
}

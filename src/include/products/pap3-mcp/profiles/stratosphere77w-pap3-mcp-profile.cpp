#include "stratosphere77w-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

Strato77WPAP3MCPProfile::Strato77WPAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [product](bool powered) {
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, powered ? 200 : 0);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, powered ? 180 : 0);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? 180 : 0);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/ap_on", [product](int engaged) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, engaged ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::CMD_B, engaged ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_CAPT, engaged ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_FO, engaged ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/at_arm", [product](int armed) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, armed ? 1 : 0);
        product->setATSolenoid(armed > 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/spd_hold", [product](int on) {
        product->setLedBrightness(PAP3MCPLed::SPEED, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/flch", [product](int on) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/hdg_sel_eng", [product](int on) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/althold", [product](int on) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Strato/777/mcp/vshold", [product](int on) {
        product->setLedBrightness(PAP3MCPLed::VS, on ? 1 : 0);
    }, this);
}

bool Strato77WPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("Strato/777/mcp/ap_on") &&
           Dataref::getInstance()->exists("Strato/B777/fms1/Line01_L");
}

const std::vector<std::string> &Strato77WPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/autopilot/autopilot_has_power",
        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",
        "Strato/777/mcp/vshold",
        "sim/cockpit2/autopilot/nav1_obs_deg_mag_pilot",
        "sim/cockpit2/autopilot/nav2_obs_deg_mag_pilot",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &Strato77WPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1
        {0, {"N1/CLB CON", "Strato/B777/button_switch/mcp/autothrottle/clbcon"}},
        {1, {"SPEED", "Strato/B777/button_switch/mcp/autothrottle/spd"}},
        {2, {"VNAV", "Strato/B777/button_switch/mcp/ap/vnav"}},
        {3, {"LVL CHG", "Strato/B777/button_switch/mcp/ap/flch"}},
        {4, {"HDG SEL", "Strato/B777/button_switch/mcp/ap/hdgSel"}},
        {5, {"LNAV", "Strato/B777/button_switch/mcp/ap/lnav"}},
        {6, {"VOR LOC", "Strato/B777/button_switch/mcp/ap/loc"}},
        {7, {"APP", "Strato/B777/button_switch/mcp/ap/app"}},

        // Row 2
        {8, {"ALT HLD", "Strato/B777/button_switch/mcp/ap/altHold"}},
        {9, {"V/S", "Strato/B777/button_switch/mcp/ap/vs"}},
        {10, {"CMD A", "Strato/B777/button_switch/mcp/ap/engage_1"}},
        {11, {"CWS A", "Strato/B777/button_switch/mcp/ap/engage_1"}},
        {12, {"CMD B", "Strato/B777/button_switch/mcp/ap/engage_2"}},
        {13, {"CWS B", "Strato/B777/button_switch/mcp/ap/engage_2"}},
        {14, {"IAS/MACH", "Strato/B777/button_switch/mcp/spd_mode"}},
        {15, {"HDG/TRK", "Strato/B777/button_switch/mcp/hdg_mode"}},

        // Row 3
        {16, {"VS/FPA", "Strato/B777/button_switch/mcp/vs_mode"}},

        // Encoder directions
        {19, {"SPD DEC", "Strato/777/spd_dn"}},
        {20, {"SPD INC", "Strato/777/spd_up"}},
        {21, {"HDG DEC", "Strato/777/hdg_dn"}},
        {22, {"HDG INC", "Strato/777/hdg_up"}},
        {23, {"ALT DEC", "Strato/777/autopilot/alt_dn"}},
        {24, {"ALT INC", "Strato/777/autopilot/alt_up"}},
        {38, {"VS DEC", "Strato/777/autopilot/vs_dn"}},
        {39, {"VS INC", "Strato/777/autopilot/vs_up"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &Strato77WPAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/autopilot/nav1_course_up", "sim/autopilot/nav1_course_down"},
        {1, "SPD", "Strato/777/spd_up", "Strato/777/spd_dn"},
        {2, "HDG", "Strato/777/hdg_up", "Strato/777/hdg_dn"},
        {3, "ALT", "Strato/777/autopilot/alt_up", "Strato/777/autopilot/alt_dn"},
        {4, "V/S", "Strato/777/autopilot/vs_up", "Strato/777/autopilot/vs_dn"},
        {5, "CRS FO", "sim/autopilot/nav2_course_up", "sim/autopilot/nav2_course_down"},
    };
    return encoders;
}

void Strato77WPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dm = Dataref::getInstance();

    bool hasPower = dm->getCached<bool>("sim/cockpit2/autopilot/autopilot_has_power");
    data.displayEnabled = hasPower;
    data.displayTest = false;
    data.showLabels = true;
    data.showDashesWhenInactive = false;
    data.showLabelsWhenInactive = true;
    data.showCourse = false;
    data.digitA = false;
    data.digitB = false;

    if (!hasPower) {
        return;
    }

    data.speed = dm->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");
    data.speedVisible = true;

    float heading = dm->getCached<float>("sim/cockpit/autopilot/heading_mag");
    data.heading = static_cast<int>(std::round(heading)) % 360;
    data.headingVisible = true;

    float altitude = dm->getCached<float>("sim/cockpit/autopilot/altitude");
    data.altitude = (altitude >= 0 && altitude < 100000)
        ? static_cast<int>(std::round(altitude / 100.0f) * 100)
        : 0;

    data.verticalSpeed = dm->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    data.verticalSpeedVisible = dm->getCached<bool>("Strato/777/mcp/vshold");

    data.crsCapt = static_cast<int>(dm->getCached<float>("sim/cockpit2/autopilot/nav1_obs_deg_mag_pilot"));
    data.crsFo = static_cast<int>(dm->getCached<float>("sim/cockpit2/autopilot/nav2_obs_deg_mag_pilot"));
}

void Strato77WPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void Strato77WPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    if (encoder->incCmd.empty() || encoder->decCmd.empty()) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void Strato77WPAP3MCPProfile::maybeToggle(const char *stateDataref, bool hwState, const char *cmd) {
    if (!Dataref::getInstance()->exists(stateDataref)) {
        return;
    }
    bool simState = Dataref::getInstance()->get<bool>(stateDataref);
    if (simState != hwState) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void Strato77WPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT: byte 0x04, bit 0x08
    if (byteOffset == 0x04 && bitMask == 0x08) {
        hwFDCaptOn = state;
        maybeToggle("Strato/777/mcp/flt_dir_pilot", hwFDCaptOn, "Strato/B777/button_switch/mcp/fd_capt");
        return;
    }

    // FD FO: byte 0x04, bit 0x20
    if (byteOffset == 0x04 && bitMask == 0x20) {
        hwFDFoOn = state;
        maybeToggle("Strato/777/mcp/flt_dir_copilot", hwFDFoOn, "Strato/B777/button_switch/mcp/fd_fo");
        return;
    }

    // A/T ARM left: byte 0x06, bit 0x01 = ARMED, bit 0x02 = DISARMED
    if (byteOffset == 0x06 && bitMask == 0x01) {
        if (state) {
            hwATOn = true;
            maybeToggle("Strato/777/mcp/at_arm", hwATOn, "Strato/B777/button_switch/mcp/at_arm");
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x02) {
        if (state) {
            hwATOn = false;
            maybeToggle("Strato/777/mcp/at_arm", hwATOn, "Strato/B777/button_switch/mcp/at_arm");
        }
        return;
    }

    // A/T ARM right: byte 0x06, bit 0x04 = ARMED (controls solenoid), bit 0x08 = DISARMED
    if (byteOffset == 0x06 && bitMask == 0x04) {
        if (state) {
            hwATOn = true;
            product->setATSolenoid(true);
            maybeToggle("Strato/777/mcp/at_arm", hwATOn, "Strato/B777/button_switch/mcp/at_arm");
        }
        return;
    }
    if (byteOffset == 0x06 && bitMask == 0x08) {
        if (state) {
            hwATOn = false;
            product->setATSolenoid(false);
            maybeToggle("Strato/777/mcp/at_arm", hwATOn, "Strato/B777/button_switch/mcp/at_arm");
        }
        return;
    }

    // AP DISC bar: byte 0x04, bit 0x80 = UP line
    if (byteOffset == 0x04 && bitMask == 0x80) {
        bool newEngaged = !state;
        if (newEngaged != hwApDiscEngaged) {
            hwApDiscEngaged = newEngaged;
            if (!hwApDiscEngaged) {
                Dataref::getInstance()->executeCommand("Strato/B777/button_switch/mcp/ap/disengage");
            }
        }
        return;
    }
}

void Strato77WPAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    // Strato 777 doesn't expose a MCP bank angle command
}

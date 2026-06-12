#include "sparky744-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

SparkyB744PAP3MCPProfile::SparkyB744PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool powered) {
        uint8_t backlight = powered ? 200 : 0;
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, backlight);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, powered ? 180 : 0);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? 180 : 0);
        product->forceStateSync();
    }, this);

    // MCP mode LEDs
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/FMA/active_pitch_mode", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::N1, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/FMA/autothrottle_mode", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::SPEED, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/vnav_state", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::VNAV, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/ias_mach/window_open", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::LVL_CHG, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/FMA/active_roll_mode", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::HDG_SEL, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/lnav_state", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::LNAV, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/nav_status", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::VORLOC, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/glideslope_status", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::APP, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/vert_spd/window_open", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::ALT_HLD, v < 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/vvi_fpm", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::VS, std::abs(v) > 10.0 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/cmd_L_mode/status", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::CMD_A, v > 0.5 ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_CAPT, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autopilot/cmd_R_mode/status", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::CMD_B, v > 0.5 ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::MA_FO, v > 0.5 ? 1 : 0);
    }, this);
    Dataref::getInstance()->monitorExistingDataref<double>("laminar/B747/autothrottle/armed", [product](double v) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, v > 0.5 ? 1 : 0);
        product->setATSolenoid(v > 0.5);
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
}

bool SparkyB744PAP3MCPProfile::IsEligible() {
    auto dr = Dataref::getInstance();
    return dr->exists("laminar/B747/fms1/Line01_L") && !dr->exists("FPS/748/simtime") && !dr->exists("SSG/748/simtime");
}

const std::vector<std::string> &SparkyB744PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> refs = {
        "sim/cockpit/electrical/avionics_on",
        "laminar/B747/autopilot/ias_dial_value",
        "laminar/B747/autopilot/ias_mach/window_open",
        "laminar/B747/autopilot/heading/degrees",
        "laminar/B747/autopilot/heading/altitude_dial_ft",
        "laminar/B747/cockpit2/autopilot/vvi_dial_fpm",
        "laminar/B747/autopilot/vert_spd/window_open",
        "sim/cockpit2/autopilot/nav1_obs_deg_mag_pilot",
        "sim/cockpit2/autopilot/nav2_obs_deg_mag_pilot",
    };
    return refs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &SparkyB744PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1
        {0, {"N1", "laminar/B747/autopilot/button_switch/thrust_mode"}},
        {1, {"SPEED", "laminar/B747/autopilot/button_switch/speed_mode"}},
        {2, {"VNAV", "laminar/B747/autopilot/button_switch/VNAV"}},
        {3, {"LVL CHG", "laminar/B747/autopilot/button_switch/flch_mode"}},
        {4, {"HDG SEL", "laminar/B747/autopilot/button_switch/heading_select"}},
        {5, {"LNAV", "laminar/B747/autopilot/button_switch/LNAV"}},
        {6, {"VOR LOC", "laminar/B747/autopilot/button_switch/loc_mode"}},
        {7, {"APP", "laminar/B747/autopilot/approach"}},

        // Row 2
        {8, {"ALT HLD", "laminar/B747/autopilot/button_switch/alt_hold_mode"}},
        {9, {"V/S", "laminar/B747/autopilot/button_switch/vs_mode"}},
        {10, {"CMD A", "laminar/B747/autopilot/button_switch/cmd_L"}},
        {12, {"CMD B", "laminar/B747/autopilot/button_switch/cmd_R"}},
        {14, {"AP DISC", "laminar/B747/autopilot/button_switch/yoke_disengage_capt"}},

        // Encoder directions
        {17, {"CRS CAPT DEC", "sim/autopilot/nav1_course_down"}},
        {18, {"CRS CAPT INC", "sim/autopilot/nav1_course_up"}},
        {19, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {20, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {21, {"HDG DEC", "sim/autopilot/heading_down"}},
        {22, {"HDG INC", "sim/autopilot/heading_up"}},
        {23, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {24, {"ALT INC", "sim/autopilot/altitude_up"}},
        {25, {"CRS FO DEC", "sim/autopilot/nav2_course_down"}},
        {26, {"CRS FO INC", "sim/autopilot/nav2_course_up"}},
        {38, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {39, {"VS INC", "sim/autopilot/vertical_speed_up"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &SparkyB744PAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/autopilot/nav1_course_up", "sim/autopilot/nav1_course_down"},
        {1, "SPD", "sim/autopilot/airspeed_up", "sim/autopilot/airspeed_down"},
        {2, "HDG", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {3, "ALT", "sim/autopilot/altitude_up", "sim/autopilot/altitude_down"},
        {4, "V/S", "sim/autopilot/vertical_speed_up", "sim/autopilot/vertical_speed_down"},
        {5, "CRS FO", "sim/autopilot/nav2_course_up", "sim/autopilot/nav2_course_down"},
    };
    return encoders;
}

void SparkyB744PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dm = Dataref::getInstance();

    data.displayEnabled = dm->getCached<bool>("sim/cockpit/electrical/avionics_on");
    data.displayTest = false;
    data.showLabels = false;

    data.digitA = false;
    data.speed = static_cast<float>(dm->getCached<double>("laminar/B747/autopilot/ias_dial_value"));
    data.speedVisible = dm->getCached<double>("laminar/B747/autopilot/ias_mach/window_open") > 0.5;

    data.heading = static_cast<int>(dm->getCached<double>("laminar/B747/autopilot/heading/degrees"));
    data.headingVisible = true;

    data.altitude = static_cast<int>(dm->getCached<double>("laminar/B747/autopilot/heading/altitude_dial_ft"));

    data.verticalSpeed = static_cast<float>(dm->getCached<double>("laminar/B747/cockpit2/autopilot/vvi_dial_fpm"));
    data.verticalSpeedVisible = dm->getCached<double>("laminar/B747/autopilot/vert_spd/window_open") > 0.5;

    data.crsCapt = static_cast<int>(dm->getCached<float>("sim/cockpit2/autopilot/nav1_obs_deg_mag_pilot"));
    data.crsFo = static_cast<int>(dm->getCached<float>("sim/cockpit2/autopilot/nav2_obs_deg_mag_pilot"));
    data.showCourse = true;
}

void SparkyB744PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void SparkyB744PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void SparkyB744PAP3MCPProfile::maybeToggle(const char *stateDataref, bool hwState, const char *toggleCmd) {
    if (!Dataref::getInstance()->exists(stateDataref)) {
        return;
    }
    bool simState = Dataref::getInstance()->get<bool>(stateDataref);
    if (simState != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

void SparkyB744PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT: byte 0x04, bit 0x08
    if (byteOffset == 0x04 && bitMask == 0x08) {
        hwFDCaptOn = state;
        maybeToggle("laminar/B747/autopilot/AFDS/status_annun_pilot", hwFDCaptOn, "laminar/B747/toggle_switch/flight_dir_L");
        return;
    }

    // FD FO: byte 0x04, bit 0x20
    if (byteOffset == 0x04 && bitMask == 0x20) {
        hwFDFoOn = state;
        maybeToggle("laminar/B747/autopilot/AFDS/status_annun_copilot", hwFDFoOn, "laminar/B747/toggle_switch/flight_dir_R");
        return;
    }

    // AP DISC: byte 0x04 bit 0x80 = bar down
    if (byteOffset == 0x04 && bitMask == 0x80) {
        if (state) {
            hwApDiscEngaged = false;
            Dataref::getInstance()->executeCommand("laminar/B747/autopilot/slide_switch/disengage_bar");
        }
        return;
    }
    if (byteOffset == 0x05 && bitMask == 0x01) {
        if (state) {
            hwApDiscEngaged = true;
        }
        return;
    }
}

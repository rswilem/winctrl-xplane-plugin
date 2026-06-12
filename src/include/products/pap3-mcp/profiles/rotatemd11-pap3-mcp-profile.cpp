#include "rotatemd11-pap3-mcp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMUtilities.h>

RotateMD11PAP3MCPProfile::RotateMD11PAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    // Monitor power and brightness - MD-11 specific behavior
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd", [this, product](bool hasPower) {
        if (hasPower) {
            // LCD at full brightness when powered
            product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 255);

            // Button backlight: 0 at startup, then adjustable via panel lights
            if (!backlightInitialized) {
                product->setLedBrightness(PAP3MCPLed::BACKLIGHT, 0);
                product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 0);
                backlightInitialized = true;
            } else {
                // Use FGS panel brightness for button backlight
                float panelLights = Dataref::getInstance()->getCached<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio");
                uint8_t brightness = std::clamp(panelLights, 0.0f, 1.0f) * 255;
                product->setLedBrightness(PAP3MCPLed::BACKLIGHT, brightness);
                product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, brightness);
            }
        } else {
            // No power - turn off all displays
            product->setLedBrightness(PAP3MCPLed::BACKLIGHT, 0);
            product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
            product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 0);
        }

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio", [](float panelLights) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    }, this);

    // MD-11 MCP has NO LED annunciators - all LEDs disabled by not setting up monitors
    // The real MD-11 uses a different display system (not LED buttons)
}

bool RotateMD11PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/systems/gcp_alt_presel_ft");
}

const std::vector<std::string> &RotateMD11PAP3MCPProfile::displayDatarefs() const {
    static std::vector<std::string> datarefs = {
        "Rotate/aircraft/systems/elec_dc_batt_bus_pwrd",
        "Rotate/aircraft/systems/light_fgs_panel_brt_ratio",
        "Rotate/aircraft/systems/gcp_spd_presel_ias",
        "Rotate/aircraft/systems/gcp_spd_presel_mach",
        "Rotate/aircraft/systems/gcp_active_ias_mach_mode",
        "Rotate/aircraft/systems/gcp_hdg_presel_deg",
        "Rotate/aircraft/systems/gcp_alt_presel_ft",
        "Rotate/aircraft/systems/gcp_vs_sel_fpm",
        "Rotate/aircraft/systems/afs_fms_spd_engaged",
        "Rotate/aircraft/systems/afs_roll_mode",
        "Rotate/aircraft/systems/gcp_hdg_trk_presel_set",
        "sim/cockpit/radios/nav1_obs_degm",
        "sim/cockpit/radios/nav2_obs_degm",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &RotateMD11PAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Row 1 (byte 0x01) - Main autopilot mode buttons
        // Note: MD-11 uses dataref writes instead of commands for buttons
        {1, {"SPEED", "Rotate/aircraft/controls/fgs_fms_spd", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {2, {"PROF", "Rotate/aircraft/controls/fgs_prof", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {3, {"LVL CHG", "Rotate/aircraft/controls/fgs_alt_mode_sel", PAP3MCPDatarefType::SET_VALUE_PHASED, -1.0}},
        {4, {"HDG SEL", "Rotate/aircraft/controls/fgs_hdg_mode_sel", PAP3MCPDatarefType::SET_VALUE_PHASED, -1.0}},
        {5, {"NAV", "Rotate/aircraft/controls/fgs_nav", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {7, {"APP", "Rotate/aircraft/controls/fgs_appr_land", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},

        // Row 2 (byte 0x02) - Additional autopilot buttons
        {8, {"ALT HLD", "Rotate/aircraft/controls/fgs_alt_mode_sel", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {9, {"V/S", "Rotate/aircraft/controls/fgs_vs_fpa", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {10, {"CMD A", "Rotate/aircraft/controls/fgs_autoflight", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {11, {"CWS A", "Rotate/aircraft/controls/fgs_afs_ovrd_off_1", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {12, {"CMD B", "Rotate/aircraft/controls/fgs_autoflight", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {13, {"CWS B", "Rotate/aircraft/controls/fgs_afs_ovrd_off_2", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {14, {"C/O", "Rotate/aircraft/controls/fgs_ias_mach", PAP3MCPDatarefType::SET_VALUE_PHASED, 1.0}},
        {15, {"SPD INTV", "Rotate/aircraft/controls/fgs_spd_sel_mode", PAP3MCPDatarefType::SET_VALUE_PHASED, -1.0}},

        // Row 3 (byte 0x03)
        {16, {"ALT INTV", "Rotate/aircraft/controls/fgs_alt_mode_sel", PAP3MCPDatarefType::SET_VALUE_PHASED, -1.0}},

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", "sim/autopilot/heading_down"}},
        {18, {"CRS CAPT INC", "sim/autopilot/heading_up"}},
        {19, {"SPD DEC", "Rotate/aircraft/controls_c/fgs_spd_sel_dn"}},
        {20, {"SPD INC", "Rotate/aircraft/controls_c/fgs_spd_sel_up"}},
        {21, {"HDG DEC", "Rotate/aircraft/controls_c/fgs_hdg_sel_dn"}},
        {22, {"HDG INC", "Rotate/aircraft/controls_c/fgs_hdg_sel_up"}},
        {23, {"ALT DEC", "Rotate/aircraft/controls_c/fgs_alt_sel_dn"}},
        {24, {"ALT INC", "Rotate/aircraft/controls_c/fgs_alt_sel_up"}},
        {25, {"CRS FO DEC", "sim/autopilot/heading_down"}},
        {26, {"CRS FO INC", "sim/autopilot/heading_up"}},
        {38, {"VS DEC", "Rotate/aircraft/controls_c/fgs_pitch_sel_dn"}},
        {39, {"VS INC", "Rotate/aircraft/controls_c/fgs_pitch_sel_up"}}};
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &RotateMD11PAP3MCPProfile::encoderDefs() const {
    static std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {1, "SPD", "Rotate/aircraft/controls_c/fgs_spd_sel_up", "Rotate/aircraft/controls_c/fgs_spd_sel_dn"},
        {2, "HDG", "Rotate/aircraft/controls_c/fgs_hdg_sel_up", "Rotate/aircraft/controls_c/fgs_hdg_sel_dn"},
        {3, "ALT", "Rotate/aircraft/controls_c/fgs_alt_sel_up", "Rotate/aircraft/controls_c/fgs_alt_sel_dn"},
        {4, "V/S (Pitch)", "Rotate/aircraft/controls_c/fgs_pitch_sel_up", "Rotate/aircraft/controls_c/fgs_pitch_sel_dn"},
        {5, "CRS FO", "sim/autopilot/heading_up", "sim/autopilot/heading_down"}};
    return encoders;
}

void RotateMD11PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();
    data.showLabels = true;             // MD-11 MCP has labels on the display
    data.showDashesWhenInactive = true; // Show dashes (---) when displays are inactive
    data.showLabelsWhenInactive = true; // Show labels even when displays are inactive
    data.showCourse = false;            // MD-11 doesn't have CRS displays on the MCP
    bool hasPower = dataref->getCached<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    data.displayEnabled = hasPower;

    if (!hasPower) {
        return;
    }

    // Speed - check if IAS or MACH mode
    int iasMachMode = dataref->getCached<int>("Rotate/aircraft/systems/gcp_active_ias_mach_mode");
    if (iasMachMode == 0) {
        // IAS mode
        data.speed = dataref->getCached<float>("Rotate/aircraft/systems/gcp_spd_presel_ias");
    } else {
        // MACH mode
        data.speed = dataref->getCached<float>("Rotate/aircraft/systems/gcp_spd_presel_mach");
    }

    // Control SPD LCD visibility based on FMS SPD engagement
    // When FMS SPD is engaged (value = 1), hide the SPD display
    int fmsSpdEngaged = dataref->getCached<int>("Rotate/aircraft/systems/afs_fms_spd_engaged");
    data.speedVisible = (fmsSpdEngaged != 1);

    // Heading
    int hdgRaw = dataref->getCached<int>("Rotate/aircraft/systems/gcp_hdg_presel_deg");
    data.heading = hdgRaw % 360;
    if (data.heading < 0) {
        data.heading += 360;
    }

    // Control HDG LCD visibility
    int hdgTrkSel = dataref->getCached<int>("Rotate/aircraft/systems/gcp_hdg_trk_presel_set");
    data.headingVisible = (hdgTrkSel == 1);

    // Altitude
    data.altitude = dataref->getCached<int>("Rotate/aircraft/systems/gcp_alt_presel_ft");

    // Vertical speed - only show when V/S is non-zero
    data.verticalSpeed = dataref->getCached<float>("Rotate/aircraft/systems/gcp_vs_sel_fpm");
    data.verticalSpeedVisible = (std::abs(data.verticalSpeed) > 0.5f);

    // Course - MD-11 doesn't have CRS displays on the MCP (using standard datarefs for reference)
    data.crsCapt = dataref->getCached<int>("sim/cockpit/radios/nav1_obs_degm");
    data.crsFo = dataref->getCached<int>("sim/cockpit/radios/nav2_obs_degm");

    // MD-11 doesn't have special digit flags
    data.digitA = false;
    data.digitB = false;
    data.displayTest = false;

    // MD-11 MCP has NO LED annunciators
    data.ledN1 = false;
    data.ledSpeed = false;
    data.ledVNAV = false;
    data.ledLvlChg = false;
    data.ledHdgSel = false;
    data.ledLNAV = false;
    data.ledVORLOC = false;
    data.ledAPP = false;
    data.ledAltHld = false;
    data.ledVS = false;
    data.ledCmdA = false;
    data.ledCwsA = false;
    data.ledCmdB = false;
    data.ledCwsB = false;
    data.ledATArm = false;
    data.ledMaCapt = false;
    data.ledMaFO = false;
}

void RotateMD11PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    // MD-11 uses dataref writes for buttons (momentary: press=1/value, release=0)
    if (button->datarefType == PAP3MCPDatarefType::SET_VALUE_PHASED) {
        Dataref::getInstance()->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? static_cast<int>(button->value) : 0);
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

void RotateMD11PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

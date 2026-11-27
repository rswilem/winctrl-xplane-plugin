#include "q4xp-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

namespace {
    constexpr const char *kPanelLights = "FJS/Q4XP/Lights/panelText_LIT";
    constexpr const char *kSpeed = "sim/cockpit/autopilot/airspeed";
    constexpr const char *kHeading = "sim/cockpit/autopilot/heading_mag";
    constexpr const char *kAltitude = "sim/cockpit/autopilot/altitude";
    constexpr const char *kVerticalSpeed = "sim/cockpit/autopilot/vertical_velocity";
    constexpr const char *kCrsCapt = "sim/cockpit2/radios/actuators/hsi_obs_deg_mag_pilot";
    constexpr const char *kCrsFo = "sim/cockpit2/radios/actuators/hsi_obs_deg_mag_copilot";
} // namespace

Q4XPPAP3MCPProfile::Q4XPPAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setATSolenoid(false);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kPanelLights, [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 1) {
            return;
        }

        uint8_t target = static_cast<uint8_t>(std::clamp(brightness[1], 0.0f, 1.0f) * 255);
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, target);
        product->forceStateSync();
    },
        this);
}

bool Q4XPPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("FJS/Q4XP/cdu1/text_line_0");
}

const std::vector<std::string> &Q4XPPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kSpeed,
        kHeading,
        kAltitude,
        kVerticalSpeed,
        kCrsCapt,
        kCrsFo,
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &Q4XPPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {0, {"N1", "FJS/Q4XP/SoftKey/ap_yd", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"VNAV", "FJS/Q4XP/SoftKey/ap_vnav", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", "FJS/Q4XP/SoftKey/ap_ias", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", "FJS/Q4XP/SoftKey/ap_hdg", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", "FJS/Q4XP/SoftKey/ap_nav", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", "FJS/Q4XP/SoftKey/ap_nav", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", "FJS/Q4XP/SoftKey/ap_appr", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", "FJS/Q4XP/SoftKey/ap_alt", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", "FJS/Q4XP/SoftKey/ap_vs", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", "FJS/Q4XP/Autopilot/FGCP_BUTTON_AP", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", "FJS/Q4XP/Autopilot/TCS_Engage", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"CMD B", "FJS/Q4XP/Autopilot/FGCP_BUTTON_AP", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {13, {"CWS B", "FJS/Q4XP/Autopilot/TCS_Engage", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {14, {"C/O", "FJS/Q4XP/SoftKey/ap_hsisel", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"SPD INTV", "FJS/Q4XP/SoftKey/ap_bc", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {16, {"ALT INTV", "FJS/Q4XP/SoftKey/ap_altsel", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},

        {17, {"CRS CAPT DEC", "sim/radios/obs_HSI_down"}},
        {18, {"CRS CAPT INC", "sim/radios/obs_HSI_up"}},
        {19, {"SPD DEC", "FJS/Q4XP/ICP1/Speed_Bug_Knob_Down"}},
        {20, {"SPD INC", "FJS/Q4XP/ICP1/Speed_Bug_Knob_Up"}},
        {21, {"HDG DEC", "sim/autopilot/heading_down"}},
        {22, {"HDG INC", "sim/autopilot/heading_up"}},
        {23, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {24, {"ALT INC", "sim/autopilot/altitude_up"}},
        {25, {"CRS FO DEC", "sim/radios/copilot_obs_HSI_down"}},
        {26, {"CRS FO INC", "sim/radios/copilot_obs_HSI_up"}},
        {38, {"VS DEC", "FJS/Q4XP/Autopilot/WheelDN"}},
        {39, {"VS INC", "FJS/Q4XP/Autopilot/WheelUP"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &Q4XPPAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/radios/obs_HSI_up", "sim/radios/obs_HSI_down"},
        {1, "SPD", "FJS/Q4XP/ICP1/Speed_Bug_Knob_Up", "FJS/Q4XP/ICP1/Speed_Bug_Knob_Down"},
        {2, "HDG", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {3, "ALT", "sim/autopilot/altitude_up", "sim/autopilot/altitude_down"},
        {4, "V/S", "FJS/Q4XP/Autopilot/WheelUP", "FJS/Q4XP/Autopilot/WheelDN"},
        {5, "CRS FO", "sim/radios/copilot_obs_HSI_up", "sim/radios/copilot_obs_HSI_down"},
    };
    return encoders;
}

void Q4XPPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dr = Dataref::getInstance();

    data.displayEnabled = true;
    data.displayTest = false;
    data.showLabels = false;
    data.showLabelsWhenInactive = false;
    data.showDashesWhenInactive = false;
    data.showCourse = true;
    data.speedVisible = true;
    data.headingVisible = true;
    data.altitudeVisible = true;
    data.verticalSpeedVisible = true;
    data.spdMach = false;
    data.headingTrk = false;
    data.vsFpa = false;
    data.digitA = false;
    data.digitB = false;

    data.speed = std::round(dr->getCached<float>(kSpeed));
    data.heading = (static_cast<int>(std::lround(dr->getCached<float>(kHeading))) % 360 + 360) % 360;
    data.altitude = static_cast<int>(std::lround(dr->getCached<float>(kAltitude)));
    data.verticalSpeed = std::round(dr->getCached<float>(kVerticalSpeed) / 100.0f) * 100.0f;
    data.crsCapt = (static_cast<int>(std::lround(dr->getCached<float>(kCrsCapt))) % 360 + 360) % 360;
    data.crsFo = (static_cast<int>(std::lround(dr->getCached<float>(kCrsFo))) % 360 + 360) % 360;
}

void Q4XPPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    } else if (phase == xplm_CommandBegin) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void Q4XPPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void Q4XPPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    auto dr = Dataref::getInstance();

    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20) && !state) {
        dr->executeCommand("FJS/Q4XP/Autopilot/FGCP_BUTTON_STBY");
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x80 && state) {
        dr->executeCommand("FJS/Q4XP/Autopilot/AUTOPILOT_DISCONNECT");
    }
}

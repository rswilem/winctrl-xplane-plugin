#include "g1000-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>

// The G1000's autopilot keys and knobs on a 737-style MCP, mapped by meaning: HDG SEL is HDG,
// LNAV and VOR LOC are NAV, LVL CHG is FLC, CWS A is the yaw damper and V/S turns NOSE UP/DN.
// The LEDs follow X-Plane's autopilot, which the G1000 drives. The windows stay blank, as the
// G1000 shows its references on its screens.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";

    constexpr const char *kServosOn = "sim/cockpit2/autopilot/servos_on";
    constexpr const char *kFlightDirectorMode = "sim/cockpit2/autopilot/flight_director_mode";
    constexpr const char *kYawDamperOn = "sim/cockpit2/switches/yaw_damper_on";
    constexpr const char *kHsiSource = "sim/cockpit2/radios/actuators/HSI_source_select_pilot";

    // The autopilot mode statuses read 0 = off, 1 = armed, 2 = active
    constexpr const char *kHeadingStatus = "sim/cockpit2/autopilot/heading_status";
    constexpr const char *kNavStatus = "sim/cockpit2/autopilot/nav_status";
    constexpr const char *kApproachStatus = "sim/cockpit2/autopilot/approach_status";
    constexpr const char *kGlideslopeStatus = "sim/cockpit2/autopilot/glideslope_status";
    constexpr const char *kVnavStatus = "sim/cockpit2/autopilot/vnav_status";
    constexpr const char *kSpeedStatus = "sim/cockpit2/autopilot/speed_status";
    constexpr const char *kAltitudeHoldStatus = "sim/cockpit2/autopilot/altitude_hold_status";
    constexpr const char *kVviStatus = "sim/cockpit2/autopilot/vvi_status";
    constexpr int kStatusActive = 2;

    // HSI_source_select_pilot: 0 = NAV1, 1 = NAV2, 2 = GPS1, 3 = GPS2
    constexpr int kHsiSourceGps1 = 2;

    const char *kStatusRefs[] = {
        kHeadingStatus,
        kNavStatus,
        kApproachStatus,
        kGlideslopeStatus,
        kVnavStatus,
        kSpeedStatus,
        kAltitudeHoldStatus,
        kVviStatus,
        kHsiSource,
    };

    // The G1000 keys have no lights of their own, so the MCP LEDs stay at full brightness
    constexpr uint8_t kLedBrightness = 255;
} // namespace

G1000PAP3MCPProfile::G1000PAP3MCPProfile(ProductPAP3MCP *product, const std::string &bezel, const G1000KnobCommands &customKnobs) :
    PAP3MCPAircraftProfile(product),
    bezel(bezel) {
    auto either = [](const std::string &command, std::string fallback) {
        return command.empty() ? fallback : command;
    };

    const G1000KnobCommands knobs = {
        .speedUp = either(customKnobs.speedUp, "sim/autopilot/airspeed_up"),
        .speedDown = either(customKnobs.speedDown, "sim/autopilot/airspeed_down"),
        .headingUp = either(customKnobs.headingUp, bezelCommand("hdg_up")),
        .headingDown = either(customKnobs.headingDown, bezelCommand("hdg_down")),
        .altitudeUp = either(customKnobs.altitudeUp, bezelCommand("alt_inner_up")),
        .altitudeDown = either(customKnobs.altitudeDown, bezelCommand("alt_inner_down")),
        .noseUp = either(customKnobs.noseUp, bezelCommand("nose_up")),
        .noseDown = either(customKnobs.noseDown, bezelCommand("nose_down")),
        // Both course knobs set the one HSI course, so both default to the bezel's CRS knob
        .course1Up = either(customKnobs.course1Up, bezelCommand("crs_up")),
        .course1Down = either(customKnobs.course1Down, bezelCommand("crs_down")),
        .course2Up = either(customKnobs.course2Up, bezelCommand("crs_up")),
        .course2Down = either(customKnobs.course2Down, bezelCommand("crs_down")),
    };

    // Keys are held for as long as the hardware key, so the cockpit key moves with it
    buttons = {
        {2, {"VNAV", bezelCommand("vnv"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", bezelCommand("flc"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", bezelCommand("hdg"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", bezelCommand("nav"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", bezelCommand("nav"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", bezelCommand("apr"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", bezelCommand("alt"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", bezelCommand("vs"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", bezelCommand("ap"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", bezelCommand("yd"), PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        // N1 (0), SPEED (1), CMD B (12), CWS B (13), C/O (14), SPD INTV (15) and ALT INTV (16) have no G1000 key

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", knobs.course1Down}},
        {18, {"CRS CAPT INC", knobs.course1Up}},
        {19, {"SPD DEC", knobs.speedDown}},
        {20, {"SPD INC", knobs.speedUp}},
        {21, {"HDG DEC", knobs.headingDown}},
        {22, {"HDG INC", knobs.headingUp}},
        {23, {"ALT DEC", knobs.altitudeDown}},
        {24, {"ALT INC", knobs.altitudeUp}},
        {25, {"CRS FO DEC", knobs.course2Down}},
        {26, {"CRS FO INC", knobs.course2Up}},
        {38, {"VS DEC", knobs.noseDown}},
        {39, {"VS INC", knobs.noseUp}},

        // 27/29: FD CAPT/FO   - handled by handleSwitchChanged (byte 0x04, bits 0x08 / 0x20)
        // 31/32: AP DISC bar  - handled by handleSwitchChanged (byte 0x05, bit 0x01 = bar down)
        // 33-37: Bank angle   - unmapped, the G1000 has no bank limit
        // 40/41: A/T ARM      - unmapped, there is no autothrottle
    };

    // The product reports the encoders as the button pairs above and never calls
    // encoderDefs() or encoderRotated(); they mirror those buttons like in the other profiles.
    encoders = {
        {0, "CRS CAPT", knobs.course1Up, knobs.course1Down},
        {1, "SPD", knobs.speedUp, knobs.speedDown},
        {2, "HDG", knobs.headingUp, knobs.headingDown},
        {3, "ALT", knobs.altitudeUp, knobs.altitudeDown},
        {4, "V/S", knobs.noseUp, knobs.noseDown},
        {5, "CRS FO", knobs.course2Up, knobs.course2Down},
    };

    auto dataref = Dataref::getInstance();

    dataref->monitorExistingDataref<std::vector<float>>(kPanelBrightness, [this](const std::vector<float> &) {
        updateBrightness();
    },
        this);

    dataref->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBrightness();
        updateLeds();
    },
        this);

    dataref->monitorExistingDataref<bool>(kServosOn, [this](bool) {
        updateLeds();
    },
        this);

    dataref->monitorExistingDataref<bool>(kYawDamperOn, [this](bool) {
        updateLeds();
    },
        this);

    for (const char *ref : kStatusRefs) {
        dataref->monitorExistingDataref<int>(ref, [this](int) {
            updateLeds();
        },
            this);
    }

    // The monitors only report changes, so a dark panel would otherwise keep the product's defaults
    updateBrightness();
    updateLeds();
}

std::string G1000PAP3MCPProfile::bezelCommand(const char *key) const {
    return "sim/GPS/" + bezel + "_" + key;
}

const std::vector<std::string> &G1000PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &G1000PAP3MCPProfile::buttonDefs() const {
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &G1000PAP3MCPProfile::encoderDefs() const {
    return encoders;
}

void G1000PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    data.displayEnabled = false;
}

void G1000PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    } else if (phase == xplm_CommandBegin) {
        // The knob steps act once per detent
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void G1000PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const std::string &command = delta > 0 ? encoder->incCmd : encoder->decCmd;
    for (int step = std::abs(static_cast<int>(delta)); step > 0; step--) {
        Dataref::getInstance()->executeCommand(command.c_str());
    }
}

void G1000PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT and FD FO both drive the G1000's single flight director. The FD key is a toggle,
    // so press it only when the flight director doesn't match the switch.
    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20)) {
        bool fdOn = Dataref::getInstance()->get<int>(kFlightDirectorMode) > 0;
        if (fdOn != state) {
            Dataref::getInstance()->executeCommand(bezelCommand("fd").c_str());
        }
        return;
    }

    // AP DISCONNECT bar down disengages the autopilot; raising it does nothing, the AP key re-engages
    if (byteOffset == 0x05 && bitMask == 0x01 && state) {
        Dataref::getInstance()->executeCommand("sim/autopilot/servos_off_any");
    }
}

void G1000PAP3MCPProfile::updateBrightness() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);

    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;

    product->setLedBrightness(PAP3MCPLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
    // The windows stay blank, so their backlight stays off
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? kLedBrightness : 0);
}

void G1000PAP3MCPProfile::updateLeds() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);

    // APR also engages NAV, but like on a 737 only APP lights for an approach
    bool approachOn = dataref->getCached<int>(kApproachStatus) > 0 || dataref->getCached<int>(kGlideslopeStatus) > 0;

    // NAV follows whichever source the HSI shows: GPS lights LNAV, a VOR or localizer VOR LOC
    bool navOn = dataref->getCached<int>(kNavStatus) > 0 && !approachOn;
    bool navIsGps = dataref->getCached<int>(kHsiSource) >= kHsiSourceGps1;

    const std::pair<PAP3MCPLed, bool> leds[] = {
        {PAP3MCPLed::HDG_SEL, dataref->getCached<int>(kHeadingStatus) > 0},
        {PAP3MCPLed::LNAV, navOn && navIsGps},
        {PAP3MCPLed::VORLOC, navOn && !navIsGps},
        {PAP3MCPLed::APP, approachOn},
        {PAP3MCPLed::VNAV, dataref->getCached<int>(kVnavStatus) > 0},
        {PAP3MCPLed::LVL_CHG, dataref->getCached<int>(kSpeedStatus) > 0},
        // Armed means the altitude preselect will be captured, not that the altitude is held
        {PAP3MCPLed::ALT_HLD, dataref->getCached<int>(kAltitudeHoldStatus) == kStatusActive},
        {PAP3MCPLed::VS, dataref->getCached<int>(kVviStatus) > 0},
        {PAP3MCPLed::CMD_A, dataref->getCached<bool>(kServosOn)},
        {PAP3MCPLed::CWS_A, dataref->getCached<bool>(kYawDamperOn)},
        // No G1000 equivalent
        {PAP3MCPLed::N1, false},
        {PAP3MCPLed::SPEED, false},
        {PAP3MCPLed::CMD_B, false},
        {PAP3MCPLed::CWS_B, false},
        {PAP3MCPLed::AT_ARM, false},
        {PAP3MCPLed::MA_CAPT, false},
        {PAP3MCPLed::MA_FO, false},
    };

    for (const auto &[led, on] : leds) {
        bool lit = powered && on;

        auto it = ledStates.find(led);
        if (it != ledStates.end() && it->second == lit) {
            continue;
        }

        ledStates[led] = lit;
        product->setLedBrightness(led, lit ? 1 : 0);
    }
}

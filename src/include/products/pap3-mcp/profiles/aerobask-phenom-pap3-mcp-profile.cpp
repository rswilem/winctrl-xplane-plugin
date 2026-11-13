#include "aerobask-phenom-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>

// Aerobask Phenom 300. Its G1000 has a GFC 700 autopilot panel, mapped onto a 737-style MCP by
// meaning: HDG SEL is HDG, LNAV and VOR LOC are NAV, LVL CHG is FLC, SPEED is CSC, CWS A is the
// yaw damper, C/O is the speed reference's IAS/Mach switch and V/S turns NOSE UP/DN. The mode keys
// work through the MFD's bezel commands, and the knobs have custom commands that also turn them in
// the cockpit. The LEDs follow X-Plane's autopilot, which the G1000 drives. The windows stay
// blank, as the G1000 shows its references on its screens.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";

    constexpr const char *kServosOn = "sim/cockpit2/autopilot/servos_on";
    constexpr const char *kFlightDirectorMode = "sim/cockpit2/autopilot/flight_director_mode";
    constexpr const char *kYawDamperOn = "sim/cockpit2/switches/yaw_damper_on";
    constexpr const char *kLowBankOn = "sim/cockpit2/annunciators/autopilot_bank_limit";
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

    // The MFD's bezel keys, which the GFC 700 keys share
    const std::string kBezel = "sim/GPS/g1000n3_";
} // namespace

AerobaskPhenomPAP3MCPProfile::AerobaskPhenomPAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product) {
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

bool AerobaskPhenomPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

const std::vector<std::string> &AerobaskPhenomPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &AerobaskPhenomPAP3MCPProfile::buttonDefs() const {
    // Keys are held for as long as the hardware key, so the cockpit key moves with it
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {1, {"SPEED", "aerobask/gfc700_csc", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"VNAV", kBezel + "vnv", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", kBezel + "flc", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", kBezel + "hdg", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", kBezel + "nav", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", kBezel + "nav", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", kBezel + "apr", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", kBezel + "alt", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", kBezel + "vs", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", kBezel + "ap", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", "sim/systems/yaw_damper_toggle", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {14, {"C/O", "sim/autopilot/knots_mach_toggle", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        // N1 (0), CMD B (12), CWS B (13), SPD INTV (15) and ALT INTV (16) have no GFC 700 key

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", "aerobask/gfc700_crs1_dec"}},
        {18, {"CRS CAPT INC", "aerobask/gfc700_crs1_inc"}},
        {19, {"SPD DEC", "aerobask/gfc700_spd_dec"}},
        {20, {"SPD INC", "aerobask/gfc700_spd_inc"}},
        {21, {"HDG DEC", "aerobask/gfc700_hdg_dec"}},
        {22, {"HDG INC", "aerobask/gfc700_hdg_inc"}},
        {23, {"ALT DEC", "aerobask/gfc700_alti_dec"}},
        {24, {"ALT INC", "aerobask/gfc700_alti_inc"}},
        {25, {"CRS FO DEC", "aerobask/gfc700_crs2_dec"}},
        {26, {"CRS FO INC", "aerobask/gfc700_crs2_inc"}},
        {38, {"VS DEC", "aerobask/gfc700_vvi_dec"}},
        {39, {"VS INC", "aerobask/gfc700_vvi_inc"}},

        // 27/29: FD CAPT/FO   - handled by handleSwitchChanged (byte 0x04, bits 0x08 / 0x20)
        // 31/32: AP DISC bar  - handled by handleSwitchChanged (byte 0x05, bit 0x01 = bar down)
        // 33-37: Bank angle   - handled by handleBankAngleSwitch
        // 40/41: A/T ARM      - unmapped, there is no autothrottle arm switch
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &AerobaskPhenomPAP3MCPProfile::encoderDefs() const {
    // The product reports the encoders as the button pairs above and never calls
    // encoderDefs() or encoderRotated(); they mirror those buttons like in the other profiles.
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "aerobask/gfc700_crs1_inc", "aerobask/gfc700_crs1_dec"},
        {1, "SPD", "aerobask/gfc700_spd_inc", "aerobask/gfc700_spd_dec"},
        {2, "HDG", "aerobask/gfc700_hdg_inc", "aerobask/gfc700_hdg_dec"},
        {3, "ALT", "aerobask/gfc700_alti_inc", "aerobask/gfc700_alti_dec"},
        {4, "V/S", "aerobask/gfc700_vvi_inc", "aerobask/gfc700_vvi_dec"},
        {5, "CRS FO", "aerobask/gfc700_crs2_inc", "aerobask/gfc700_crs2_dec"},
    };
    return encoders;
}

void AerobaskPhenomPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    data.displayEnabled = false;
}

void AerobaskPhenomPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
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

void AerobaskPhenomPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const std::string &command = delta > 0 ? encoder->incCmd : encoder->decCmd;
    for (int step = std::abs(static_cast<int>(delta)); step > 0; step--) {
        Dataref::getInstance()->executeCommand(command.c_str());
    }
}

void AerobaskPhenomPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT and FD FO both drive the G1000's single flight director. The FD key is a toggle,
    // so press it only when the flight director doesn't match the switch.
    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20)) {
        bool fdOn = Dataref::getInstance()->get<int>(kFlightDirectorMode) > 0;
        if (fdOn != state) {
            Dataref::getInstance()->executeCommand((kBezel + "fd").c_str());
        }
        return;
    }

    // AP DISCONNECT bar down disengages the autopilot; raising it does nothing, the AP key re-engages
    if (byteOffset == 0x05 && bitMask == 0x01 && state) {
        Dataref::getInstance()->executeCommand("sim/autopilot/servos_off_any");
    }
}

void AerobaskPhenomPAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    // The GFC 700 only has a low bank mode: the 10° and 15° positions turn it on, 20° to 30°
    // turn it off. Only act when the switch moves between the two halves, and not on the
    // position reported when the profile loads.
    int lowBank = (switchByte & (0x02 | 0x04)) != 0 ? 1 : 0;
    bool firstReport = hwLowBank < 0;
    if (lowBank == hwLowBank) {
        return;
    }

    hwLowBank = lowBank;
    bool lowBankOn = Dataref::getInstance()->get<int>(kLowBankOn) != 0;
    if (!firstReport && lowBankOn != (lowBank == 1)) {
        Dataref::getInstance()->executeCommand("sim/autopilot/bank_limit_toggle");
    }
}

void AerobaskPhenomPAP3MCPProfile::updateBrightness() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);

    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;

    product->setLedBrightness(PAP3MCPLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
    // The windows stay blank, so their backlight stays off
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? kLedBrightness : 0);
}

void AerobaskPhenomPAP3MCPProfile::updateLeds() {
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
        // CSC has no state in X-Plane's autopilot to follow
        {PAP3MCPLed::SPEED, false},
        // No GFC 700 equivalent
        {PAP3MCPLed::N1, false},
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

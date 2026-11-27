#include "dwsim-atr72-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <string>

// DeltaWing Simulations ATR 72-600. Its FGCP is mapped onto the 737 keys by meaning: LVL CHG is
// IAS, HDG SEL is HDG, LNAV and VOR LOC are NAV (the NAV SOURCE knob picks FMS or V/ILS), ALT HLD
// is ALT, CMD A is AP, CWS A is YD and CMD B is CPL, which couples the autopilot to the first
// officer's side. There is no autothrottle; the SPD knob sets the ICP speed target and SPD INTV
// works its AUTO/MAN key, like the 737's switch between the FMC speed and the MCP speed.
// The SPD, HDG, ALT and CRS knobs and the V/S pitch wheel turn the cockpit knobs.
// N1, SPEED, C/O, CWS B and ALT INTV have no FGCP key, and BC and STBY no 737 key.
//
// The aircraft exports its FGCP targets and states for hardware (DWSim/ATR_Series/afcs/*). The
// FGCP itself only lights the AP, YD and CPL arrows, which CMD A, CWS A and CMD B follow. The modes
// show on the FMA, which the export gives as text, so the mode LEDs follow the FMA like a 737's
// light its modes: lateral modes when armed or active, vertical modes when active (VNAV also when
// armed), and APP for any approach with a vertical path.

namespace {
    // The FGCP and the captain's ICP each have their own power
    constexpr const char *kFgcpOn = "DWSim/ATR_Series/afcs/fgcp_on";
    constexpr const char *kIcpOn = "DWSim/ATR_Series/afcs/icp_on_capt";
    // Main instrument panel and pedestal lighting, 0 to 1, which also lights the glareshield
    constexpr const char *kPanelBrightness = "DWSim/ATR_Series/lit/cockpit/mipPed/brightness";

    constexpr const char *kSpeedTarget = "DWSim/ATR_Series/afcs/spd_target_kt";
    // AUTO flies the FMS speed, MAN the ICP speed target
    constexpr const char *kSpeedAuto = "DWSim/ATR_Series/afcs/spd_mode_auto";
    constexpr const char *kHeadingSelected = "DWSim/ATR_Series/afcs/hdg_sel_deg";
    constexpr const char *kAltitudeSelected = "DWSim/ATR_Series/afcs/alt_sel_ft";
    constexpr const char *kAltitudeValid = "DWSim/ATR_Series/afcs/alt_sel_valid";
    constexpr const char *kVerticalSpeedTarget = "DWSim/ATR_Series/afcs/vs_target_fpm";
    constexpr const char *kVerticalSpeedActive = "DWSim/ATR_Series/afcs/vs_active";
    constexpr const char *kCourseCapt = "DWSim/ATR_Series/afcs/crs_capt_deg";
    constexpr const char *kCourseFo = "DWSim/ATR_Series/afcs/crs_fo_deg";

    constexpr const char *kAutopilotEngaged = "DWSim/ATR_Series/afcs/ap_engaged";
    constexpr const char *kYawDamperEngaged = "DWSim/ATR_Series/afcs/yd_engaged";
    constexpr const char *kFlightDirectorCapt = "DWSim/ATR_Series/afcs/fd_capt";
    constexpr const char *kFlightDirectorFo = "DWSim/ATR_Series/afcs/fd_fo";
    // The CPL arrow pointing to the first officer, lit with the panel lighting
    constexpr const char *kLampCoupledFo = "DWSim/ATR_Series/lit/front/afcs/cplRight";
    constexpr float kLampLit = 0.05f;

    // The FMA as text, empty when no mode is armed or active. A captured mode carries a star (LOC*,
    // ALT*). Lateral: WING LVL, HDG HOLD, ROLL HOLD, HDG SEL, LNAV, VOR, LOC, BC, L-LOC. Vertical:
    // PITCH HOLD, ALT, VS, IAS, VNAV IAS / ALT / VS / PATH, PROT LEVEL, GS, L-GS, V-FP, GA; the
    // armed vertical text can hold ALT SEL next to a second mode.
    constexpr const char *kFmaLateralActive = "DWSim/ATR_Series/afcs/fma_lat_active";
    constexpr const char *kFmaLateralArmed = "DWSim/ATR_Series/afcs/fma_lat_armed";
    constexpr const char *kFmaVerticalActive = "DWSim/ATR_Series/afcs/fma_vert_active";
    constexpr const char *kFmaVerticalArmed = "DWSim/ATR_Series/afcs/fma_vert_armed";

    const char *kStateRefs[] = {
        kAutopilotEngaged,
        kYawDamperEngaged,
    };

    const char *kFmaRefs[] = {
        kFmaLateralActive,
        kFmaLateralArmed,
        kFmaVerticalActive,
        kFmaVerticalArmed,
    };

    bool contains(const std::string &text, const char *mode) {
        return text.find(mode) != std::string::npos;
    }

    const std::string kFgcp = "DWSim/ATR_Series/commands/front/afcs/";
    const std::string kIcp = "DWSim/ATR_Series/commands/front/icp/left/";
    // The yoke's quick disconnect: drops the autopilot and keeps the yaw damper
    constexpr const char *kQuickDisconnect = "DWSim/ATR_Series/commands/yoke/captain/redPushButton";
} // namespace

DWSimATR72PAP3MCPProfile::DWSimATR72PAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product) {
    auto dataref = Dataref::getInstance();

    dataref->monitorExistingDataref<float>(kPanelBrightness, [this](float) {
        updateBrightness();
    },
        this);

    dataref->monitorExistingDataref<bool>(kFgcpOn, [this](bool) {
        updateBrightness();
        updateLeds();
    },
        this);

    for (const char *ref : kStateRefs) {
        dataref->monitorExistingDataref<int>(ref, [this](int) {
            updateLeds();
        },
            this);
    }

    for (const char *ref : kFmaRefs) {
        dataref->monitorExistingDataref<std::string>(ref, [this](const std::string &) {
            updateLeds();
        },
            this);
    }

    dataref->monitorExistingDataref<float>(kLampCoupledFo, [this](float) {
        updateLeds();
    },
        this);

    // The monitors only report changes, so a dark panel would otherwise keep the product's defaults
    updateBrightness();
    updateLeds();
}

bool DWSimATR72PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("DWSim/ATR_Series/afcs/alt_sel_ft");
}

const std::vector<std::string> &DWSimATR72PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kFgcpOn,
        kIcpOn,
        kSpeedTarget,
        kSpeedAuto,
        kHeadingSelected,
        kAltitudeSelected,
        kAltitudeValid,
        kVerticalSpeedTarget,
        kVerticalSpeedActive,
        kCourseCapt,
        kCourseFo,
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &DWSimATR72PAP3MCPProfile::buttonDefs() const {
    // Keys are held for as long as the hardware key, so the cockpit key moves with it
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {2, {"VNAV", kFgcp + "vnavPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", kFgcp + "iasPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", kFgcp + "hdgPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", kFgcp + "navPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", kFgcp + "navPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", kFgcp + "appPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", kFgcp + "altPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", kFgcp + "vsPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", kFgcp + "apPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", kFgcp + "ydPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {12, {"CMD B", kFgcp + "cplPushButton", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"SPD INTV", kIcp + "autoManPushButton/toggle", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        // N1 (0), SPEED (1), CWS B (13), C/O (14) and ALT INTV (16) have no FGCP key

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", kFgcp + "courseKnobLeft/rotateLeft"}},
        {18, {"CRS CAPT INC", kFgcp + "courseKnobLeft/rotateRight"}},
        {19, {"SPD DEC", kIcp + "spdTgtRotationKnob/rotateLeft"}},
        {20, {"SPD INC", kIcp + "spdTgtRotationKnob/rotateRight"}},
        {21, {"HDG DEC", kFgcp + "headingKnob/rotateLeft"}},
        {22, {"HDG INC", kFgcp + "headingKnob/rotateRight"}},
        {23, {"ALT DEC", kFgcp + "altSel/rotateLeft"}},
        {24, {"ALT INC", kFgcp + "altSel/rotateRight"}},
        {25, {"CRS FO DEC", kFgcp + "courseKnobRight/rotateLeft"}},
        {26, {"CRS FO INC", kFgcp + "courseKnobRight/rotateRight"}},
        {38, {"VS DEC", kFgcp + "vsSpeed/rotateDn"}},
        {39, {"VS INC", kFgcp + "vsSpeed/rotateUp"}},

        // 27/29: FD CAPT/FO   - handled by handleSwitchChanged (byte 0x04, bits 0x08 / 0x20)
        // 31/32: AP DISC bar  - handled by handleSwitchChanged (byte 0x05, bit 0x01 = bar down)
        // 33-37: Bank angle   - unmapped, the FGCP has no bank limit selector
        // 40/41: A/T ARM      - unmapped, there is no autothrottle
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &DWSimATR72PAP3MCPProfile::encoderDefs() const {
    // The product reports the encoders as the button pairs above and never calls
    // encoderDefs() or encoderRotated(); they mirror those buttons like in the other profiles.
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", kFgcp + "courseKnobLeft/rotateRight", kFgcp + "courseKnobLeft/rotateLeft"},
        {1, "SPD", kIcp + "spdTgtRotationKnob/rotateRight", kIcp + "spdTgtRotationKnob/rotateLeft"},
        {2, "HDG", kFgcp + "headingKnob/rotateRight", kFgcp + "headingKnob/rotateLeft"},
        {3, "ALT", kFgcp + "altSel/rotateRight", kFgcp + "altSel/rotateLeft"},
        {4, "V/S", kFgcp + "vsSpeed/rotateUp", kFgcp + "vsSpeed/rotateDn"},
        {5, "CRS FO", kFgcp + "courseKnobRight/rotateRight", kFgcp + "courseKnobRight/rotateLeft"},
    };
    return encoders;
}

void DWSimATR72PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();

    data.displayEnabled = dataref->getCached<bool>(kFgcpOn);
    data.showLabels = false;

    // Like the 737's window with the FMC speed, the speed window blanks while AUTO flies the FMS
    // speed and the SPD knob has no effect
    data.speed = std::round(dataref->getCached<float>(kSpeedTarget));
    data.speedVisible = dataref->getCached<bool>(kIcpOn) && !dataref->getCached<bool>(kSpeedAuto);
    data.spdMach = false;

    data.heading = static_cast<int>(std::lround(dataref->getCached<float>(kHeadingSelected))) % 360;
    data.crsCapt = static_cast<int>(std::lround(dataref->getCached<float>(kCourseCapt))) % 360;
    data.crsFo = static_cast<int>(std::lround(dataref->getCached<float>(kCourseFo))) % 360;

    data.altitude = static_cast<int>(std::lround(dataref->getCached<float>(kAltitudeSelected)));
    data.altitudeVisible = dataref->getCached<bool>(kAltitudeValid);

    // The pitch wheel only sets a vertical speed in V/S mode, as on the 737
    data.verticalSpeed = std::round(dataref->getCached<float>(kVerticalSpeedTarget));
    data.verticalSpeedVisible = dataref->getCached<bool>(kVerticalSpeedActive);
}

void DWSimATR72PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
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

void DWSimATR72PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const std::string &command = delta > 0 ? encoder->incCmd : encoder->decCmd;
    for (int step = std::abs(static_cast<int>(delta)); step > 0; step--) {
        Dataref::getInstance()->executeCommand(command.c_str());
    }
}

void DWSimATR72PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT and FD FO work the captain's and first officer's FD keys. The keys are toggles,
    // so press one only when its flight director doesn't match the switch.
    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20)) {
        bool captain = bitMask == 0x08;
        bool fdOn = Dataref::getInstance()->get<int>(captain ? kFlightDirectorCapt : kFlightDirectorFo) != 0;
        if (fdOn != state) {
            Dataref::getInstance()->executeCommand((kFgcp + (captain ? "left/fdPush" : "right/fdPush")).c_str());
        }
        return;
    }

    // AP DISCONNECT bar down disengages the autopilot with the quick disconnect, pressed only while
    // the autopilot is engaged; raising it does nothing, the AP key re-engages
    if (byteOffset == 0x05 && bitMask == 0x01 && state) {
        if (Dataref::getInstance()->get<int>(kAutopilotEngaged) != 0) {
            Dataref::getInstance()->executeCommand(kQuickDisconnect);
        }
    }
}

void DWSimATR72PAP3MCPProfile::updateBrightness() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kFgcpOn);

    float level = dataref->getCached<float>(kPanelBrightness);
    level = powered && std::isfinite(level) ? std::clamp(level, 0.0f, 1.0f) : 0.0f;

    product->setLedBrightness(PAP3MCPLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, powered ? 255 : 0);
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? 255 : 0);
}

void DWSimATR72PAP3MCPProfile::updateLeds() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kFgcpOn);

    std::string lateralActive = dataref->getCached<std::string>(kFmaLateralActive);
    std::string lateral = lateralActive + " " + dataref->getCached<std::string>(kFmaLateralArmed);
    std::string verticalActive = dataref->getCached<std::string>(kFmaVerticalActive);
    std::string vertical = verticalActive + " " + dataref->getCached<std::string>(kFmaVerticalArmed);

    // APP arms LOC + GS, L-LOC + L-GS or V-FP; like on a 737, only APP lights for an approach
    bool approach = contains(vertical, "GS") || contains(vertical, "V-FP") || contains(lateral, "L-LOC");
    bool radioNav = contains(lateral, "VOR") || contains(lateral, "LOC") || contains(lateral, "BC");

    const std::pair<PAP3MCPLed, bool> leds[] = {
        {PAP3MCPLed::CMD_A, dataref->getCached<int>(kAutopilotEngaged) != 0},
        {PAP3MCPLed::CWS_A, dataref->getCached<int>(kYawDamperEngaged) != 0},
        {PAP3MCPLed::CMD_B, dataref->getCached<float>(kLampCoupledFo) > kLampLit},
        // HDG HOLD is a basic mode, like the 737's CWS R, so only HDG SEL lights
        {PAP3MCPLed::HDG_SEL, lateralActive.rfind("HDG SEL", 0) == 0},
        {PAP3MCPLed::LNAV, contains(lateral, "LNAV")},
        {PAP3MCPLed::VORLOC, radioNav && !approach},
        {PAP3MCPLed::APP, approach},
        // ALT* is the capture, which like the 737's ALT ACQ leaves ALT HLD dark
        {PAP3MCPLed::ALT_HLD, verticalActive == "ALT"},
        {PAP3MCPLed::VS, verticalActive == "VS"},
        {PAP3MCPLed::LVL_CHG, verticalActive == "IAS"},
        {PAP3MCPLed::VNAV, contains(vertical, "VNAV")},
        // No FGCP equivalent
        {PAP3MCPLed::SPEED, false},
        {PAP3MCPLed::N1, false},
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

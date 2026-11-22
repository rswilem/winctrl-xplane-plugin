#include "tu-154-ce-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

// The ABSU-154 has no MCP of its own, so its panel is mapped onto the 737 keys by meaning:
// HDG SEL is ZK (selected heading), LNAV is NVU, VOR LOC is LOC (ZAKHOD) and APP is the glideslope,
// ALT HLD, LVL CHG and C/O hold the altitude, the IAS and the Mach, V/S and VNAV are AZ-I and AZ-II,
// and CMD A and B are STAB. CWS A and B flip the director's NAV and LDG switches.
// The CRS knobs set the Kurs-MP 1 and 2 courses and HDG the captain's PNP course, each shown in its
// own window. ALT sets the ZK heading, while its window shows the VBE's altitude. SPD works the
// IAS/M knob and V/S turns the pitch wheel.
// The aircraft's mode buttons are momentary, so its commands are held for as long as the
// hardware key.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    // The ABSU panel sits on the pedestal; its lighting runs from 0 to 1.1
    constexpr const char *kPedestalLighting = "tu154b2/custom/lights/mid_left_panel_int";
    constexpr float kPedestalLightingMax = 1.1f;

    // The mode lamps, lit with a brightness that follows the bus voltage
    constexpr const char *kLampAt = "tu154b2/custom/lights/button/absu_stab_spd";
    constexpr const char *kLampAz1 = "tu154b2/custom/lights/button/absu_az1";
    constexpr const char *kLampAz2 = "tu154b2/custom/lights/button/absu_az2";
    constexpr const char *kLampStabV = "tu154b2/custom/lights/button/absu_stab_v";
    constexpr const char *kLampZk = "tu154b2/custom/lights/button/absu_zk";
    constexpr const char *kLampNvu = "tu154b2/custom/lights/button/absu_nvu";
    constexpr const char *kLampLoc = "tu154b2/custom/lights/button/absu_app";
    constexpr const char *kLampGs = "tu154b2/custom/lights/button/absu_gz";
    constexpr const char *kLampStabH = "tu154b2/custom/lights/button/absu_stab_h";
    constexpr float kLampLit = 0.05f;

    // 0 = off, 1 = yoke (dampers only), 2 = autopilot. The STAB lamp is never lit, so CMD A and B follow these.
    constexpr const char *kRollMode = "tu154b2/custom/absu/roll_main_mode";
    constexpr const char *kPitchMode = "tu154b2/custom/absu/pitch_main_mode";
    constexpr int kModeAutopilot = 2;

    // Maintained switches, read by the aircraft every frame. Its commands for them only hold them on.
    constexpr const char *kRollChannel = "tu154b2/custom/switchers/console/absu_roll_ch_on";
    constexpr const char *kPitchChannel = "tu154b2/custom/switchers/console/absu_pitch_ch_on";
    constexpr const char *kDirectorNav = "tu154b2/custom/switchers/console/absu_nav_on";
    constexpr const char *kDirectorLanding = "tu154b2/custom/switchers/console/absu_landing_on";
    constexpr const char *kDirectorNeedles = "tu154b2/custom/switchers/console/absu_needles_on";
    constexpr const char *kAutothrottlePrepare = "tu154b2/custom/switchers/console/absu_speed_prepare";

    // 0 = off, 1 = on, 2 = ready, 3 = holding the speed, 4 = go-around
    constexpr const char *kAutothrottleMode = "tu154b2/custom/absu/stu_mode";
    constexpr int kAutothrottleHolding = 3;
    // 1 = the captain's speed marker drives the autothrottle, 0 = the first officer's
    constexpr const char *kAutothrottleSource = "tu154b2/custom/switchers/console/absu_speed_us_right_left";
    // The speed markers are needle angles on the KUS IAS gauge: 10° at 150 km/h, 260° more at 800 km/h
    constexpr const char *kSpeedMarkerLeft = "tu154b2/custom/gauges/speed/ias_yellow_left";
    constexpr const char *kSpeedMarkerRight = "tu154b2/custom/gauges/speed/ias_yellow_right";
    constexpr float kMarkerDegreesPerKmh = 260.0f / 650.0f;
    // The IAS/M knob: while the autothrottle holds the speed, the ABSU moves the speed marker 6 km/h
    // a second, so an SPD detent sets about 1 km/h
    constexpr const char *kSpeedChange = "tu154b2/custom/switchers/console/absu_speed_change";
    constexpr float kSpeedChangeHold = 1.0f / 6.0f;

    // The pitch wheel, 1 nose up and -1 nose down: the ABSU turns it 10 units a second, each 0.2° of
    // pitch, so a V/S detent sets 0.2°
    constexpr const char *kPitchWheelTurn = "tu154b2/custom/switchers/console/absu_pitch_wheel_dir";
    constexpr float kPitchWheelHold = 0.1f;

    // The Kurs-MP course selectors follow X-Plane's NAV 1 and NAV 2 OBS
    constexpr const char *kKursMp1Course = "sim/cockpit2/radios/actuators/nav1_obs_deg_mag_pilot";
    constexpr const char *kKursMp2Course = "sim/cockpit2/radios/actuators/nav2_obs_deg_mag_pilot";
    // The captain's PNP course knob, read by the aircraft as its movement
    constexpr const char *kPnpCourseKnob = "tu154b2/custom/gauges/compas/pkp_obs_knob_L";
    constexpr const char *kPnpCourse = "tu154b2/custom/gauges/compas/pkp_obs_set_L";
    // The altitude the captain's VBE indicates, in metres and shown in feet in feet mode
    constexpr const char *kIndicatedAltitude = "tu154b2/custom/gauges/alt/vbe_alt_left";
    constexpr const char *kAltimeterInFeet = "tu154b2/custom/gauges/alt/vbe_mode_left";
    constexpr float kMetresPerFoot = 0.3048f;

    const char *kLampRefs[] = {
        kLampAt,
        kLampAz1,
        kLampAz2,
        kLampStabV,
        kLampZk,
        kLampNvu,
        kLampLoc,
        kLampGs,
        kLampStabH,
    };

    const char *kStateRefs[] = {
        kRollMode,
        kPitchMode,
        kDirectorNav,
        kDirectorLanding,
    };
} // namespace

Tu154CEPAP3MCPProfile::Tu154CEPAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product),
    speedChange{kSpeedChange, kSpeedChangeHold},
    pitchWheel{kPitchWheelTurn, kPitchWheelHold} {
    auto dataref = Dataref::getInstance();

    dataref->monitorExistingDataref<float>(kPedestalLighting, [this](float) {
        updateBrightness();
    },
        this);

    dataref->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBrightness();
        updateLeds();
    },
        this);

    for (const char *ref : kLampRefs) {
        dataref->monitorExistingDataref<float>(ref, [this](float) {
            updateLeds();
        },
            this);
    }

    for (const char *ref : kStateRefs) {
        dataref->monitorExistingDataref<int>(ref, [this](int) {
            updateLeds();
        },
            this);
    }

    // The A/T ARM switch is held up while the autothrottle is prepared, and drops when the aircraft cancels it
    dataref->monitorExistingDataref<int>(kAutothrottlePrepare, [this](int prepared) {
        this->product->setATSolenoid(prepared == 1);
        updateLeds();
    },
        this);

    // Scheduled by each knob detent
    XPLMRegisterFlightLoopCallback(releaseSwitch, 0.0f, &speedChange);
    XPLMRegisterFlightLoopCallback(releaseSwitch, 0.0f, &pitchWheel);

    // The monitors only report changes, so a dark panel would otherwise keep the product's defaults
    updateBrightness();
    updateLeds();
}

Tu154CEPAP3MCPProfile::~Tu154CEPAP3MCPProfile() {
    // A knob turned just before the profile went would otherwise keep its control moving
    for (HeldSwitch *held : {&speedChange, &pitchWheel}) {
        XPLMUnregisterFlightLoopCallback(releaseSwitch, held);
        releaseSwitch(0.0f, 0.0f, 0, held);
    }
}

bool Tu154CEPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("tu154b2/custom/buttons/console/absu_zk");
}

const std::vector<std::string> &Tu154CEPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kAvionicsOn,
        kKursMp1Course,
        kKursMp2Course,
        kPnpCourse,
        kIndicatedAltitude,
        kAltimeterInFeet,
        kAutothrottleMode,
        kAutothrottleSource,
        kSpeedMarkerLeft,
        kSpeedMarkerRight,
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &Tu154CEPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {1, {"SPEED", "t154/absu/at", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {2, {"VNAV", "t154/absu/az2", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {3, {"LVL CHG", "t154/absu/v", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {4, {"HDG SEL", "t154/absu/zk", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {5, {"LNAV", "t154/absu/nvu", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {6, {"VOR LOC", "t154/absu/app", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {7, {"APP", "t154/absu/gs", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {8, {"ALT HLD", "t154/absu/h", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {9, {"V/S", "t154/absu/az1", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {10, {"CMD A", "t154/absu/stab", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {11, {"CWS A", kDirectorNav, PAP3MCPDatarefType::TOGGLE_VALUE}},
        {12, {"CMD B", "t154/absu/stab", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {13, {"CWS B", kDirectorLanding, PAP3MCPDatarefType::TOGGLE_VALUE}},
        {14, {"C/O", "t154/absu/m", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {15, {"SPD INTV", "t154/absu/reset", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        // N1 (0) and ALT INTV (16) have no ABSU button

        // Encoder rotations (exposed as button pairs by the hardware)
        {17, {"CRS CAPT DEC", "sim/radios/obs1_down"}},
        {18, {"CRS CAPT INC", "sim/radios/obs1_up"}},
        {19, {"SPD DEC", kSpeedChange, PAP3MCPDatarefType::ADJUST_VALUE, -1.0}},
        {20, {"SPD INC", kSpeedChange, PAP3MCPDatarefType::ADJUST_VALUE, 1.0}},
        {21, {"HDG DEC", kPnpCourseKnob, PAP3MCPDatarefType::ADJUST_VALUE, -1.0}},
        {22, {"HDG INC", kPnpCourseKnob, PAP3MCPDatarefType::ADJUST_VALUE, 1.0}},
        {23, {"ALT DEC", "sim/autopilot/heading_down"}},
        {24, {"ALT INC", "sim/autopilot/heading_up"}},
        {25, {"CRS FO DEC", "sim/radios/obs2_down"}},
        {26, {"CRS FO INC", "sim/radios/obs2_up"}},
        {38, {"VS DEC", kPitchWheelTurn, PAP3MCPDatarefType::ADJUST_VALUE, -1.0}},
        {39, {"VS INC", kPitchWheelTurn, PAP3MCPDatarefType::ADJUST_VALUE, 1.0}},

        // 27/29: FD CAPT/FO   - handled by handleSwitchChanged (byte 0x04, bits 0x08 / 0x20)
        // 31/32: AP DISC bar  - handled by handleSwitchChanged (byte 0x04, bit 0x80 = bar up; byte 0x05, bit 0x01 = bar down)
        // 33-37: Bank angle   - unmapped, the ABSU has no bank limit
        // 40/41: A/T ARM      - handled by handleSwitchChanged (byte 0x06, bits 0x01 / 0x02)
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &Tu154CEPAP3MCPProfile::encoderDefs() const {
    // The product reports the encoders as the button pairs above and never calls
    // encoderDefs() or encoderRotated(); the knobs with commands mirror those buttons.
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/radios/obs1_up", "sim/radios/obs1_down"},
        {3, "ALT", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {5, "CRS FO", "sim/radios/obs2_up", "sim/radios/obs2_down"},
    };
    return encoders;
}

void Tu154CEPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();

    data.displayEnabled = dataref->getCached<bool>(kAvionicsOn);
    data.showLabels = false;

    data.crsCapt = static_cast<int>(std::lround(dataref->getCached<float>(kKursMp1Course))) % 360;
    data.crsFo = static_cast<int>(std::lround(dataref->getCached<float>(kKursMp2Course))) % 360;
    data.heading = static_cast<int>(std::lround(dataref->getCached<float>(kPnpCourse))) % 360;

    float altitudeMetres = dataref->getCached<float>(kIndicatedAltitude);
    bool inFeet = dataref->getCached<int>(kAltimeterInFeet) == 1;
    data.altitude = static_cast<int>(std::lround(inFeet ? altitudeMetres / kMetresPerFoot : altitudeMetres));

    // The speed marker only sets the speed while the autothrottle holds it; otherwise it follows the IAS
    bool captainMarker = dataref->getCached<int>(kAutothrottleSource) == 1;
    float markerAngle = dataref->getCached<float>(captainMarker ? kSpeedMarkerLeft : kSpeedMarkerRight);
    data.speed = std::round(150.0f + (markerAngle - 10.0f) / kMarkerDegreesPerKmh);
    data.speedVisible = dataref->getCached<int>(kAutothrottleMode) >= kAutothrottleHolding;

    // The pitch wheel has no readout
    data.verticalSpeedVisible = false;
}

void Tu154CEPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dataref = Dataref::getInstance();
    if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        dataref->executeCommand(button->dataref.c_str(), phase);
        return;
    }

    // Everything else acts once per press or knob detent
    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->datarefType == PAP3MCPDatarefType::TOGGLE_VALUE) {
        dataref->set<int>(button->dataref.c_str(), dataref->get<int>(button->dataref.c_str()) ? 0 : 1);
    } else if (button->dataref == kPitchWheelTurn) {
        holdSwitch(pitchWheel, static_cast<int>(button->value));
    } else if (button->dataref == kSpeedChange) {
        holdSwitch(speedChange, static_cast<int>(button->value));
    } else if (button->datarefType == PAP3MCPDatarefType::ADJUST_VALUE) {
        // The PNP course knob wraps on its own
        dataref->set<float>(button->dataref.c_str(), dataref->get<float>(button->dataref.c_str()) + static_cast<float>(button->value));
    } else {
        dataref->executeCommand(button->dataref.c_str());
    }
}

void Tu154CEPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const std::string &command = delta > 0 ? encoder->incCmd : encoder->decCmd;
    for (int step = std::abs(static_cast<int>(delta)); step > 0; step--) {
        Dataref::getInstance()->executeCommand(command.c_str());
    }
}

void Tu154CEPAP3MCPProfile::holdSwitch(HeldSwitch &held, int direction) {
    Dataref::getInstance()->set<int>(held.dataref, direction);
    XPLMSetFlightLoopCallbackInterval(releaseSwitch, held.holdTime, 1, &held);
}

float Tu154CEPAP3MCPProfile::releaseSwitch(float, float, int, void *refcon) {
    Dataref::getInstance()->set<int>(static_cast<HeldSwitch *>(refcon)->dataref, 0);
    return 0.0f;
}

void Tu154CEPAP3MCPProfile::setChannels(bool on) {
    Dataref::getInstance()->set<int>(kRollChannel, on ? 1 : 0);
    Dataref::getInstance()->set<int>(kPitchChannel, on ? 1 : 0);
}

void Tu154CEPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    // FD CAPT and FD FO both drive the one director switch
    if (byteOffset == 0x04 && (bitMask == 0x08 || bitMask == 0x20)) {
        Dataref::getInstance()->set<int>(kDirectorNeedles, state ? 1 : 0);
        return;
    }

    // The AP DISCONNECT bar works the roll and pitch channel switches: down disengages the autopilot,
    // up switches the channels back on for STAB to engage
    if (byteOffset == 0x04 && bitMask == 0x80 && state) {
        setChannels(true);
        return;
    }
    if (byteOffset == 0x05 && bitMask == 0x01 && state) {
        setChannels(false);
        return;
    }

    // A/T ARM prepares the autothrottle; the SPEED key then engages it
    if (byteOffset == 0x06 && (bitMask == 0x01 || bitMask == 0x02) && state) {
        Dataref::getInstance()->set<int>(kAutothrottlePrepare, bitMask == 0x01 ? 1 : 0);
    }
}

void Tu154CEPAP3MCPProfile::updateBrightness() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = std::clamp(dataref->getCached<float>(kPedestalLighting) / kPedestalLightingMax, 0.0f, 1.0f);

    product->setLedBrightness(PAP3MCPLed::BACKLIGHT, powered ? static_cast<uint8_t>(level * 255) : 0);
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, powered ? 255 : 0);
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, powered ? 255 : 0);
}

void Tu154CEPAP3MCPProfile::updateLeds() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);

    auto lamp = [dataref](const char *ref) {
        return dataref->getCached<float>(ref) > kLampLit;
    };
    bool autopilot = dataref->getCached<int>(kRollMode) == kModeAutopilot || dataref->getCached<int>(kPitchMode) == kModeAutopilot;

    const std::pair<PAP3MCPLed, bool> leds[] = {
        {PAP3MCPLed::SPEED, lamp(kLampAt)},
        {PAP3MCPLed::VNAV, lamp(kLampAz2)},
        {PAP3MCPLed::LVL_CHG, lamp(kLampStabV)},
        {PAP3MCPLed::HDG_SEL, lamp(kLampZk)},
        {PAP3MCPLed::LNAV, lamp(kLampNvu)},
        {PAP3MCPLed::VORLOC, lamp(kLampLoc)},
        {PAP3MCPLed::APP, lamp(kLampGs)},
        {PAP3MCPLed::ALT_HLD, lamp(kLampStabH)},
        {PAP3MCPLed::VS, lamp(kLampAz1)},
        {PAP3MCPLed::CMD_A, autopilot},
        {PAP3MCPLed::CMD_B, autopilot},
        {PAP3MCPLed::CWS_A, dataref->getCached<int>(kDirectorNav) == 1},
        {PAP3MCPLed::CWS_B, dataref->getCached<int>(kDirectorLanding) == 1},
        {PAP3MCPLed::AT_ARM, dataref->getCached<int>(kAutothrottlePrepare) == 1},
        // No ABSU equivalent
        {PAP3MCPLed::N1, false},
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

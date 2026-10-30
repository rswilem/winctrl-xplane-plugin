#include "felis-742-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>

// The 747 classic mode select panel differs from a 737 MCP: pitch and roll
// modes are two rotary selectors and the A/P is engaged with two OFF/MAN/CMD
// paddles. The mode keys move the selectors and their LEDs show the selector position.

namespace {
    constexpr const char *kPitchModeSel = "B742/AP_panel/AP_pitch_mode_sel";
    constexpr const char *kNavModeSel = "B742/AP_panel/AP_nav_mode_sel";
    constexpr const char *kAltitudeModeSw = "B742/AP_panel/altitude_mode_sw";
    constexpr const char *kEngageA = "B742/AP_panel/AP_engage_A";
    constexpr const char *kEngageB = "B742/AP_panel/AP_engage_B";
    constexpr const char *kATOnSw = "B742/AP_panel/AT_on_sw";
    constexpr const char *kFlightDirSw = "B742/AP_panel/flight_dir_on_sw";

    const char *kIntRefs[] = {
        kPitchModeSel,
        kNavModeSel,
        kEngageA,
        kEngageB,
        "B742/EPRL/mode_epr_button",
        "B742/EPRL/mode_speed_button",
    };

    const char *kFloatRefs[] = {
        "B742/front_lamps/AP_alt_hold_lit",
        "B742/front_lamps/AP_AT_on_lit",
    };
} // namespace

Felis742PAP3MCPProfile::Felis742PAP3MCPProfile(ProductPAP3MCP *product) :
    PAP3MCPAircraftProfile(product) {
    product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 255);
    product->setLedBrightness(PAP3MCPLed::MA_CAPT, 0);
    product->setLedBrightness(PAP3MCPLed::MA_FO, 0);

    // The windows are mechanical counters, readable whatever the panel lighting,
    // so only the panel backlight follows the knob
    product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 200);
    Dataref::getInstance()->monitorExistingDataref<float>("B742/int_light/AP_int_light", [product](float brightness) {
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    },
        this);

    for (const char *ref : kIntRefs) {
        Dataref::getInstance()->monitorExistingDataref<int>(ref, [this](int) {
            updateLeds();
        },
            this);
    }

    for (const char *ref : kFloatRefs) {
        Dataref::getInstance()->monitorExistingDataref<float>(ref, [this](float) {
            updateLeds();
        },
            this);
    }

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this](bool) {
        updateLeds();
    },
        this);

    // The A/T switch is held ON by a solenoid; drop the magnetic A/T ARM switch when it trips
    Dataref::getInstance()->monitorExistingDataref<int>(kATOnSw, [product](int on) {
        if (!on) {
            product->setATSolenoid(false);
        }
    },
        this);

    updateLeds();
}

bool Felis742PAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("B742/controls/park_brake_lever");
}

const std::vector<std::string> &Felis742PAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "B742/AP_panel/AT_spd_set_rotary",
        "B742/AP_panel/heading_set",
        "B742/AP_panel/altitude_set",
        "B742/AP_panel/course_1_set",
        "B742/AP_panel/course_2_set",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &Felis742PAP3MCPProfile::buttonDefs() const {
    // SET_VALUE keys move a cockpit selector or switch to the given position
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // A/T MODE pushbuttons
        {0, {"N1", "B742/command/EPRL_mode_EPR", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {1, {"SPEED", "B742/command/EPRL_mode_SPEED", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},
        {14, {"C/O", "B742/command/EPRL_mode_MACH", PAP3MCPDatarefType::EXECUTE_CMD_PHASED}},

        // SPEED MODE selector
        {2, {"VNAV", kPitchModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(PitchMode::Mach)}},
        {3, {"LVL CHG", kPitchModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(PitchMode::IAS)}},
        {9, {"V/S", kPitchModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(PitchMode::VS)}},

        // NAV MODE selector
        {4, {"HDG SEL", kNavModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(NavMode::HDG)}},
        {5, {"LNAV", kNavModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(NavMode::INS)}},
        {6, {"VOR LOC", kNavModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(NavMode::VORLOC)}},
        {7, {"APP", kNavModeSel, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(NavMode::ILS)}},

        // Altitude mode switch
        {8, {"ALT HLD", kAltitudeModeSw, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(AltitudeMode::Hold)}},
        {16, {"ALT INTV", kAltitudeModeSw, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(AltitudeMode::Sel)}},

        // Autopilot engage paddles
        {10, {"CMD A", kEngageA, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(EngagePaddle::Cmd)}},
        {11, {"CWS A", kEngageA, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(EngagePaddle::Man)}},
        {12, {"CMD B", kEngageB, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(EngagePaddle::Cmd)}},
        {13, {"CWS B", kEngageB, PAP3MCPDatarefType::SET_VALUE, static_cast<double>(EngagePaddle::Man)}},

        {15, {"SPD INTV", ""}},

        // Encoder button-press events
        {17, {"CRS CAPT DEC", "B742/command/AP_course_1_left"}},
        {18, {"CRS CAPT INC", "B742/command/AP_course_1_right"}},
        {19, {"SPD DEC", "B742/command/AT_speed_down"}},
        {20, {"SPD INC", "B742/command/AT_speed_up"}},
        {21, {"HDG DEC", "B742/command/AP_hdg_left"}},
        {22, {"HDG INC", "B742/command/AP_hdg_right"}},
        {23, {"ALT DEC", "B742/command/AP_ALT_sel_down"}},
        {24, {"ALT INC", "B742/command/AP_ALT_sel_up"}},
        {25, {"CRS FO DEC", "B742/command/AP_course_2_left"}},
        {26, {"CRS FO INC", "B742/command/AP_course_2_right"}},
        {38, {"VS DEC", "B742/command/AP_VS_DOWN"}},
        {39, {"VS INC", "B742/command/AP_VS_UP"}},
    };
    return buttons;
}

// The product reports the encoders as the button pairs above and never calls
// encoderDefs() or encoderRotated(); they mirror those buttons like in the other profiles.
const std::vector<PAP3MCPEncoderDef> &Felis742PAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "B742/command/AP_course_1_right", "B742/command/AP_course_1_left"},
        {1, "SPD", "B742/command/AT_speed_up", "B742/command/AT_speed_down"},
        {2, "HDG", "B742/command/AP_hdg_right", "B742/command/AP_hdg_left"},
        {3, "ALT", "B742/command/AP_ALT_sel_up", "B742/command/AP_ALT_sel_down"},
        {4, "V/S", "B742/command/AP_VS_UP", "B742/command/AP_VS_DOWN"},
        {5, "CRS FO", "B742/command/AP_course_2_right", "B742/command/AP_course_2_left"},
    };
    return encoders;
}

void Felis742PAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();

    // The windows are mechanical counters, so they show with or without power
    data.displayEnabled = true;
    data.showLabels = false;
    data.speed = dataref->getCached<float>("B742/AP_panel/AT_spd_set_rotary");
    data.heading = static_cast<int>(std::lround(dataref->getCached<float>("B742/AP_panel/heading_set")));
    data.altitude = static_cast<int>(std::lround(dataref->getCached<float>("B742/AP_panel/altitude_set")));
    data.crsCapt = static_cast<int>(std::lround(dataref->getCached<float>("B742/AP_panel/course_1_set")));
    data.crsFo = static_cast<int>(std::lround(dataref->getCached<float>("B742/AP_panel/course_2_set")));

    // The vertical speed wheel has no window
    data.verticalSpeedVisible = false;
}

void Felis742PAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::SET_VALUE) {
        selectPosition(button);
    }
}

void Felis742PAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = (delta > 0) ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));

    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void Felis742PAP3MCPProfile::selectPosition(const PAP3MCPButtonDef *button) {
    const std::string &selector = button->dataref;
    int target = static_cast<int>(button->value);

    if (selector == kPitchModeSel) {
        selectPitchMode(static_cast<PitchMode>(target));
    } else if (selector == kNavModeSel) {
        selectNavMode(static_cast<NavMode>(target));
    } else if (selector == kAltitudeModeSw) {
        selectAltitudeMode(static_cast<AltitudeMode>(target));
    } else if (selector == kEngageA || selector == kEngageB) {
        moveEngagePaddle(selector, static_cast<EngagePaddle>(target));
    }
}

void Felis742PAP3MCPProfile::selectPitchMode(PitchMode target) {
    PitchMode current = static_cast<PitchMode>(Dataref::getInstance()->get<int>(kPitchModeSel));

    // Pressing the lit key again returns the selector to OFF
    if (current == target) {
        target = PitchMode::Off;
    }

    switch (target) {
        case PitchMode::VS:
            Dataref::getInstance()->executeCommand("B742/command/AP_pitch_mode_VS");
            break;
        case PitchMode::IAS:
            Dataref::getInstance()->executeCommand("B742/command/AP_pitch_mode_IAS");
            break;
        case PitchMode::Mach:
            Dataref::getInstance()->executeCommand("B742/command/AP_pitch_mode_MACH");
            break;
        case PitchMode::Turb:
            Dataref::getInstance()->executeCommand("B742/command/AP_pitch_mode_TURB");
            break;
        case PitchMode::Off:
            Dataref::getInstance()->executeCommand("B742/command/AP_pitch_mode_OFF");
            break;
    }
}

void Felis742PAP3MCPProfile::selectNavMode(NavMode target) {
    NavMode current = static_cast<NavMode>(Dataref::getInstance()->get<int>(kNavModeSel));

    // APP selects ILS, and LAND when pressed again
    if (target == NavMode::ILS && current == NavMode::ILS) {
        target = NavMode::LAND;
    }

    switch (target) {
        case NavMode::INS:
            Dataref::getInstance()->executeCommand("B742/command/AP_nav_select_INS");
            break;
        case NavMode::HDG:
            Dataref::getInstance()->executeCommand("B742/command/AP_nav_select_HDG");
            break;
        case NavMode::VORLOC:
            Dataref::getInstance()->executeCommand("B742/command/AP_nav_select_VOR_LOC");
            break;
        case NavMode::ILS:
            Dataref::getInstance()->executeCommand("B742/command/AP_nav_select_ILS");
            break;
        case NavMode::LAND:
            Dataref::getInstance()->executeCommand("B742/command/AP_nav_select_LAND");
            break;
    }
}

void Felis742PAP3MCPProfile::selectAltitudeMode(AltitudeMode target) {
    AltitudeMode current = static_cast<AltitudeMode>(Dataref::getInstance()->get<int>(kAltitudeModeSw));

    // Pressing the lit key again switches the altitude mode off
    if (current == target) {
        target = AltitudeMode::Off;
    }

    switch (target) {
        case AltitudeMode::Hold:
            Dataref::getInstance()->executeCommand("B742/command/AP_ALT_HOLD");
            break;
        case AltitudeMode::Sel:
            Dataref::getInstance()->executeCommand("B742/command/AP_ALT_SEL");
            break;
        case AltitudeMode::Off:
            Dataref::getInstance()->executeCommand("B742/command/AP_ALT_OFF");
            break;
    }
}

void Felis742PAP3MCPProfile::moveEngagePaddle(const std::string &paddle, EngagePaddle target) {
    EngagePaddle current = static_cast<EngagePaddle>(Dataref::getInstance()->get<int>(paddle.c_str()));

    // Pressing the lit key again disengages that autopilot
    if (current == target) {
        target = EngagePaddle::Off;
    }

    // The paddle only moves one position per command
    bool isPaddleA = paddle == kEngageA;
    const char *moveUp = isPaddleA ? "B742/command/AP_A_engage_up" : "B742/command/AP_B_engage_up";
    const char *moveDown = isPaddleA ? "B742/command/AP_A_engage_down" : "B742/command/AP_B_engage_down";

    for (int position = static_cast<int>(current); position < static_cast<int>(target); position++) {
        Dataref::getInstance()->executeCommand(moveUp);
    }
    for (int position = static_cast<int>(current); position > static_cast<int>(target); position--) {
        Dataref::getInstance()->executeCommand(moveDown);
    }
}

void Felis742PAP3MCPProfile::setFlightDirector(int side, bool on) {
    auto dataref = Dataref::getInstance();
    std::vector<int> switches = dataref->get<std::vector<int>>(kFlightDirSw);
    if (side >= static_cast<int>(switches.size()) || switches[side] == (on ? 1 : 0)) {
        return;
    }

    switches[side] = on ? 1 : 0;
    dataref->set<std::vector<int>>(kFlightDirSw, switches);
}

void Felis742PAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    auto dataref = Dataref::getInstance();

    // FD CAPT/FO: the Felis FD switches latch like the hardware ones
    if (byteOffset == 0x04 && bitMask == 0x08) {
        setFlightDirector(0, state);
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x20) {
        setFlightDirector(1, state);
        return;
    }

    // A/T ARM: bit 0x01 = ARMED line, bit 0x02 = DISARMED line
    if (byteOffset == 0x06 && (bitMask == 0x01 || bitMask == 0x02)) {
        if (state) {
            bool armed = bitMask == 0x01;
            product->setATSolenoid(armed);
            if ((dataref->get<int>(kATOnSw) != 0) != armed) {
                dataref->executeCommand("B742/command/AT_on_off");
            }
        }
        return;
    }

    // AP DISCONNECT bar down: the 747 classic has no bar, so use the yoke A/P disconnect
    if (byteOffset == 0x05 && bitMask == 0x01 && state) {
        dataref->executeCommand("B742/command/AP_OFF");
    }
}

void Felis742PAP3MCPProfile::updateLeds() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>("sim/cockpit/electrical/avionics_on");
    PitchMode pitchMode = static_cast<PitchMode>(dataref->getCached<int>(kPitchModeSel));
    NavMode navMode = static_cast<NavMode>(dataref->getCached<int>(kNavModeSel));
    EngagePaddle engageA = static_cast<EngagePaddle>(dataref->getCached<int>(kEngageA));
    EngagePaddle engageB = static_cast<EngagePaddle>(dataref->getCached<int>(kEngageB));

    const std::pair<PAP3MCPLed, bool> leds[] = {
        {PAP3MCPLed::N1, powered && dataref->getCached<int>("B742/EPRL/mode_epr_button") != 0},
        {PAP3MCPLed::SPEED, powered && dataref->getCached<int>("B742/EPRL/mode_speed_button") != 0},
        {PAP3MCPLed::VNAV, powered && pitchMode == PitchMode::Mach},
        {PAP3MCPLed::LVL_CHG, powered && pitchMode == PitchMode::IAS},
        {PAP3MCPLed::VS, powered && pitchMode == PitchMode::VS},
        {PAP3MCPLed::LNAV, powered && navMode == NavMode::INS},
        {PAP3MCPLed::HDG_SEL, powered && navMode == NavMode::HDG},
        {PAP3MCPLed::VORLOC, powered && navMode == NavMode::VORLOC},
        {PAP3MCPLed::APP, powered && (navMode == NavMode::ILS || navMode == NavMode::LAND)},
        {PAP3MCPLed::ALT_HLD, dataref->getCached<float>("B742/front_lamps/AP_alt_hold_lit") > 0.5f},
        {PAP3MCPLed::CMD_A, powered && engageA == EngagePaddle::Cmd},
        {PAP3MCPLed::CWS_A, powered && engageA == EngagePaddle::Man},
        {PAP3MCPLed::CMD_B, powered && engageB == EngagePaddle::Cmd},
        {PAP3MCPLed::CWS_B, powered && engageB == EngagePaddle::Man},
        {PAP3MCPLed::AT_ARM, dataref->getCached<float>("B742/front_lamps/AP_AT_on_lit") > 0.5f},
    };

    for (const auto &[led, lit] : leds) {
        auto it = ledStates.find(led);
        if (it != ledStates.end() && it->second == lit) {
            continue;
        }

        ledStates[led] = lit;
        product->setLedBrightness(led, lit ? 1 : 0);
    }
}

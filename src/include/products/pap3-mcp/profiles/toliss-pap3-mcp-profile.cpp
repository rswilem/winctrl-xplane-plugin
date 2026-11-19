#include "toliss-pap3-mcp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-pap3-mcp.h"

#include <algorithm>
#include <cmath>
#include <XPLMUtilities.h>

TolissPAP3MCPProfile::TolissPAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("AirbusFBW/SupplLightLevelRehostats", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");

        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, hasPower ? std::clamp(brightness[0], 0.0f, 1.0f) * 255 : 0);

        uint8_t ledBrightness = Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 0 ? 60 : 255;
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, hasPower ? ledBrightness : 0);

        constexpr uint8_t minimumScreenBrightness = 64;
        uint8_t screenBrightness = hasPower ? minimumScreenBrightness + std::clamp(brightness[1], 0.0f, 1.0f) * (255 - minimumScreenBrightness) : 0;
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/AnnunMode", [](int annunMode) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/SupplLightLevelRehostats");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP1Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AP2Engage");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ATHRmode");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/LOCilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/APPRilluminated");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/HDGmanaged");
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/ALTmanaged");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/AnnunMode");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP1Engage", [this](bool engaged) {
        setLed(PAP3MCPLed::CMD_A, engaged);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/AP2Engage", [this](bool engaged) {
        setLed(PAP3MCPLed::CMD_B, engaged);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("AirbusFBW/ATHRmode", [this, product](int mode) {
        setLed(PAP3MCPLed::N1, mode > 0);
        setLed(PAP3MCPLed::AT_ARM, mode > 0);
        product->setATSolenoid(mode > 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/LOCilluminated", [this](bool illuminated) {
        setLed(PAP3MCPLed::VORLOC, illuminated);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/APPRilluminated", [this](bool illuminated) {
        setLed(PAP3MCPLed::APP, illuminated);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/HDGmanaged", [this](bool managed) {
        setLed(PAP3MCPLed::LNAV, managed);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/ALTmanaged", [this](bool managed) {
        setLed(PAP3MCPLed::VNAV, managed);
    },
        this);
}

bool TolissPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/FCUAvail") &&
           Dataref::getInstance()->exists("AirbusFBW/NDrangeCapt");
}

const std::vector<std::string> &TolissPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "AirbusFBW/FCUAvail",
        "AirbusFBW/AnnunMode",

        "sim/cockpit2/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/airspeed_is_mach",
        "AirbusFBW/SPDdashed",

        "sim/cockpit/autopilot/heading_mag",
        "AirbusFBW/HDGdashed",
        "AirbusFBW/HDGTRKmode",

        "toliss_airbus/pfdoutputs/general/ap_altitude_reference",

        "sim/cockpit/autopilot/vertical_velocity",
        "AirbusFBW/VSdashed",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &TolissPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        {0, {"N1", "AirbusFBW/ATHRbutton"}},
        {1, {"SPEED", "AirbusFBW/PullSPDSel"}},
        {2, {"VNAV", "AirbusFBW/PushAltitude"}},
        {3, {"LVL CHG", "AirbusFBW/PullAltitude"}},
        {4, {"HDG SEL", "AirbusFBW/PullHDGSel"}},
        {5, {"LNAV", "AirbusFBW/PushHDGSel"}},
        {6, {"VOR LOC", "AirbusFBW/LOCbutton"}},
        {7, {"APP", "AirbusFBW/APPRbutton"}},

        {8, {"ALT HLD", "AirbusFBW/PushVSSel"}},
        {9, {"V/S", "AirbusFBW/PullVSSel"}},
        {10, {"CMD A", "toliss_airbus/ap1_push"}},
        {12, {"CMD B", "toliss_airbus/ap2_push"}},
        {14, {"C/O", "toliss_airbus/ias_mach_button_push"}},
        {15, {"SPD INTV", "AirbusFBW/PushSPDSel"}},

        {16, {"ALT INTV", "AirbusFBW/EXPEDbutton"}},

        {19, {"SPD DEC", "sim/autopilot/airspeed_down"}},
        {20, {"SPD INC", "sim/autopilot/airspeed_up"}},
        {21, {"HDG DEC", "sim/autopilot/heading_down"}},
        {22, {"HDG INC", "sim/autopilot/heading_up"}},
        {23, {"ALT DEC", "sim/autopilot/altitude_down"}},
        {24, {"ALT INC", "sim/autopilot/altitude_up"}},
        {38, {"VS DEC", "sim/autopilot/vertical_speed_down"}},
        {39, {"VS INC", "sim/autopilot/vertical_speed_up"}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &TolissPAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {1, "SPD", "sim/autopilot/airspeed_up", "sim/autopilot/airspeed_down"},
        {2, "HDG", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {3, "ALT", "sim/autopilot/altitude_up", "sim/autopilot/altitude_down"},
        {4, "V/S", "sim/autopilot/vertical_speed_up", "sim/autopilot/vertical_speed_down"},
    };
    return encoders;
}

void TolissPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dataref = Dataref::getInstance();

    data.showLabels = true;
    data.showLabelsWhenInactive = true;
    data.showDashesWhenInactive = true;
    data.showCourse = false;
    data.machDigits = 2;

    bool hasPower = dataref->getCached<bool>("AirbusFBW/FCUAvail");
    data.displayEnabled = hasPower;
    data.displayTest = hasPower && dataref->getCached<int>("AirbusFBW/AnnunMode") == 2;

    data.speed = dataref->getCached<float>("sim/cockpit2/autopilot/airspeed_dial_kts_mach");
    data.spdMach = dataref->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    data.speedVisible = data.speed > 0 && !dataref->getCached<bool>("AirbusFBW/SPDdashed");

    float heading = dataref->getCached<float>("sim/cockpit/autopilot/heading_mag");
    data.heading = static_cast<int>(heading) % 360;
    data.headingVisible = heading >= 0 && !dataref->getCached<bool>("AirbusFBW/HDGdashed");

    float altitude = dataref->getCached<float>("toliss_airbus/pfdoutputs/general/ap_altitude_reference");
    data.altitude = static_cast<int>(altitude);
    data.altitudeVisible = altitude >= 0;

    bool trkFpa = dataref->getCached<bool>("AirbusFBW/HDGTRKmode");
    data.headingTrk = trkFpa;
    data.vsFpa = trkFpa;

    float vs = dataref->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    data.verticalSpeed = trkFpa ? vs / 1000.0f : std::round(vs);
    data.verticalSpeedVisible = !dataref->getCached<bool>("AirbusFBW/VSdashed");
}

void TolissPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str());
}

void TolissPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const char *cmd = delta > 0 ? encoder->incCmd.c_str() : encoder->decCmd.c_str();
    int steps = std::abs(static_cast<int>(delta));
    for (int i = 0; i < steps; i++) {
        Dataref::getInstance()->executeCommand(cmd);
    }
}

void TolissPAP3MCPProfile::handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
    if (byteOffset == 0x04 && bitMask == 0x08) {
        syncFlightDirector("AirbusFBW/FD1Engage", state, "toliss_airbus/fd1_push");
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x20) {
        syncFlightDirector("AirbusFBW/FD2Engage", state, "toliss_airbus/fd2_push");
        return;
    }

    if (byteOffset == 0x06 && bitMask == 0x01 && state) {
        setAutothrust(true);
        return;
    }

    if (byteOffset == 0x06 && bitMask == 0x02 && state) {
        setAutothrust(false);
        return;
    }

    if (byteOffset == 0x04 && bitMask == 0x80 && state) {
        auto dataref = Dataref::getInstance();
        if (dataref->get<bool>("AirbusFBW/AP1Engage") || dataref->get<bool>("AirbusFBW/AP2Engage")) {
            dataref->executeCommand("toliss_airbus/ap_disc_left_stick");
        }
        return;
    }
}

void TolissPAP3MCPProfile::syncFlightDirector(const char *dataref, bool hwState, const char *toggleCmd) {
    if (Dataref::getInstance()->get<bool>(dataref) != hwState) {
        Dataref::getInstance()->executeCommand(toggleCmd);
    }
}

void TolissPAP3MCPProfile::setAutothrust(bool armed) {
    auto dataref = Dataref::getInstance();
    bool active = dataref->get<int>("AirbusFBW/ATHRmode") > 0;

    if (armed) {
        product->setATSolenoid(true);
    }

    if (active != armed) {
        dataref->executeCommand("AirbusFBW/ATHRbutton");
    }

    AppState::getInstance()->executeAfter(1000, this, [this]() {
        product->setATSolenoid(Dataref::getInstance()->get<int>("AirbusFBW/ATHRmode") > 0);
    });
}

void TolissPAP3MCPProfile::setLed(PAP3MCPLed led, bool on) {
    product->setLedBrightness(led, on || isAnnunTest() ? 1 : 0);
}

bool TolissPAP3MCPProfile::isAnnunTest() {
    return Dataref::getInstance()->get<int>("AirbusFBW/AnnunMode") == 2 && Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
}

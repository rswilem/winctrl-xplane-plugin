#include "xcrafts-erj-pap3-mcp-profile.h"

#include "dataref.h"
#include "product-pap3-mcp.h"

#include <cmath>
#include <cstring>
#include <XPLMUtilities.h>

XCraftsErjPAP3MCPProfile::XCraftsErjPAP3MCPProfile(ProductPAP3MCP *product) : PAP3MCPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 12) {
            return;
        }
        uint8_t target = static_cast<uint8_t>(brightness[12] * 255);
        product->setLedBrightness(PAP3MCPLed::BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, target);
        product->setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, target);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit/autopilot/autopilot_mode", [product](int mode) {
        bool engaged = (mode == 2);
        product->setLedBrightness(PAP3MCPLed::CMD_A, engaged ? 1 : 0);
        product->setLedBrightness(PAP3MCPLed::CMD_B, engaged ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/autothrottle_armed", [product](int armed) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, armed == 1 ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/autopilot/autothrottle_system_active", [product](int active) {
        product->setLedBrightness(PAP3MCPLed::AT_ARM, active == 1 ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/st55_nav", [product](int armed) {
        product->setLedBrightness(PAP3MCPLed::LNAV, armed == 1 ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/VNAV_armed", [product](int armed) {
        product->setLedBrightness(PAP3MCPLed::VNAV, armed == 1 ? 1 : 0);
    }, this);
}

bool XCraftsErjPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/ERJ/MFD1/WX_TERR_status");
}

const std::vector<std::string> &XCraftsErjPAP3MCPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "XCrafts/ERJ/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/airspeed_is_mach",
        "sim/cockpit/autopilot/heading_mag",
        "XCrafts/ERJ/autopilot/altitude",
        "XCrafts/ERJ/autopilot/vertical_velocity",
        "sim/cockpit/radios/nav1_obs_degm",
        "sim/cockpit/radios/nav2_obs_degm",
        "sim/cockpit/autopilot/autopilot_mode",
        "XCrafts/ERJ/autothrottle_armed",
        "XCrafts/ERJ/autopilot/autothrottle_system_active",
        "sim/cockpit2/autopilot/st55_nav",
        "XCrafts/ERJ/VNAV_armed",
        "XCrafts/ERJ/cockpit/annunciators_test",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, PAP3MCPButtonDef> &XCraftsErjPAP3MCPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons = {
        // Mode buttons
        {2, {"VNAV", "XCrafts/ERJ/VNAV"}},
        {3, {"FLCH", "XCrafts/ERJ/FLCH"}},
        {4, {"HDG SEL", "sim/autopilot/heading_sync"}},
        {5, {"LNAV", "XCrafts/ERJ/LNAV"}},
        {6, {"VOR LOC", "XCrafts/ERJ/LNAV"}},
        {7, {"APP", "XCrafts/ERJ/APPCH"}},
        {8, {"ALT HLD", "XCrafts/ERJ/alt_hold"}},
        {9, {"V/S", "XCrafts/ERJ/VS"}},
        {10, {"CMD A", "XCrafts/APYD_Toggle"}},
        {12, {"CMD B", "XCrafts/APYD_Toggle"}},

        // Encoder button-press events
        {17, {"CRS CAPT DEC", "sim/radios/obs1_down"}},
        {18, {"CRS CAPT INC", "sim/radios/obs1_up"}},
        {19, {"SPD DEC", "XCrafts/ERJ/autopilot/airspeed_dial_kts_mach", PAP3MCPDatarefType::ADJUST_VALUE, -1.0}},
        {20, {"SPD INC", "XCrafts/ERJ/autopilot/airspeed_dial_kts_mach", PAP3MCPDatarefType::ADJUST_VALUE, 1.0}},
        {21, {"HDG DEC", "sim/autopilot/heading_down"}},
        {22, {"HDG INC", "sim/autopilot/heading_up"}},
        {23, {"ALT DEC", "XCrafts/ERJ/autopilot/altitude", PAP3MCPDatarefType::ADJUST_VALUE, -100.0}},
        {24, {"ALT INC", "XCrafts/ERJ/autopilot/altitude", PAP3MCPDatarefType::ADJUST_VALUE, 100.0}},
        {25, {"CRS FO DEC", "sim/radios/obs2_down"}},
        {26, {"CRS FO INC", "sim/radios/obs2_up"}},
        {38, {"VS DEC", "XCrafts/ERJ/autopilot/vertical_velocity", PAP3MCPDatarefType::ADJUST_VALUE, -100.0}},
        {39, {"VS INC", "XCrafts/ERJ/autopilot/vertical_velocity", PAP3MCPDatarefType::ADJUST_VALUE, 100.0}},
    };
    return buttons;
}

const std::vector<PAP3MCPEncoderDef> &XCraftsErjPAP3MCPProfile::encoderDefs() const {
    static const std::vector<PAP3MCPEncoderDef> encoders = {
        {0, "CRS CAPT", "sim/radios/obs1_up", "sim/radios/obs1_down"},
        {1, "SPD", "dataref:XCrafts/ERJ/autopilot/airspeed_dial_kts_mach:1.0", "dataref:XCrafts/ERJ/autopilot/airspeed_dial_kts_mach:-1.0"},
        {2, "HDG", "sim/autopilot/heading_up", "sim/autopilot/heading_down"},
        {3, "ALT", "dataref:XCrafts/ERJ/autopilot/altitude:100.0", "dataref:XCrafts/ERJ/autopilot/altitude:-100.0"},
        {4, "V/S", "dataref:XCrafts/ERJ/autopilot/vertical_velocity:100.0", "dataref:XCrafts/ERJ/autopilot/vertical_velocity:-100.0"},
        {5, "CRS FO", "sim/radios/obs2_up", "sim/radios/obs2_down"},
    };
    return encoders;
}

void XCraftsErjPAP3MCPProfile::updateDisplayData(PAP3MCPDisplayData &data) {
    auto dr = Dataref::getInstance();

    data.displayEnabled = true;
    data.displayTest = dr->getCached<bool>("XCrafts/ERJ/cockpit/annunciators_test");
    data.showLabels = false;
    data.showCourse = true;

    bool isMach = dr->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speedRaw = dr->getCached<float>("XCrafts/ERJ/autopilot/airspeed_dial_kts_mach");

    if (isMach && speedRaw > 0) {
        int machHundredths = static_cast<int>(std::round(speedRaw * 100));
        data.speed = static_cast<float>(machHundredths) / 100.0f;
    } else {
        data.speed = speedRaw;
    }

    float headingRaw = dr->getCached<float>("sim/cockpit/autopilot/heading_mag");
    data.heading = (static_cast<int>(headingRaw) % 360 + 360) % 360;

    float altRaw = dr->getCached<float>("XCrafts/ERJ/autopilot/altitude");
    data.altitude = (static_cast<int>(altRaw) / 100) * 100;

    float vsRaw = dr->getCached<float>("XCrafts/ERJ/autopilot/vertical_velocity");
    data.verticalSpeed = static_cast<float>((static_cast<int>(vsRaw) / 100) * 100);

    data.crsCapt = static_cast<int>(dr->getCached<float>("sim/cockpit/radios/nav1_obs_degm"));
    data.crsFo = static_cast<int>(dr->getCached<float>("sim/cockpit/radios/nav2_obs_degm"));

    data.speedVisible = true;
    data.headingVisible = true;
    data.verticalSpeedVisible = true;

    bool apEngaged = dr->getCached<int>("sim/cockpit/autopilot/autopilot_mode") == 2;
    data.ledCmdA = apEngaged;
    data.ledCmdB = apEngaged;
    data.ledATArm = dr->getCached<int>("XCrafts/ERJ/autothrottle_armed") == 1 ||
                    dr->getCached<int>("XCrafts/ERJ/autopilot/autothrottle_system_active") == 1;
    data.ledLNAV = dr->getCached<int>("sim/cockpit2/autopilot/st55_nav") == 1;
    data.ledVNAV = dr->getCached<int>("XCrafts/ERJ/VNAV_armed") == 1;
}

void XCraftsErjPAP3MCPProfile::buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dr = Dataref::getInstance();

    if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::ADJUST_VALUE) {
        float current = dr->get<float>(button->dataref.c_str());
        dr->set<float>(button->dataref.c_str(), current + static_cast<float>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_ONCE) {
        dr->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == PAP3MCPDatarefType::EXECUTE_CMD_PHASED) {
        dr->executeCommand(button->dataref.c_str(), phase);
    }
}

void XCraftsErjPAP3MCPProfile::encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) {
    if (!encoder || delta == 0) {
        return;
    }

    const std::string &cmd = (delta > 0) ? encoder->incCmd : encoder->decCmd;
    int steps = std::abs(static_cast<int>(delta));

    static const std::string prefix = "dataref:";
    if (cmd.compare(0, prefix.size(), prefix) == 0) {
        size_t colonPos = cmd.find(':', prefix.size());
        if (colonPos == std::string::npos) {
            return;
        }
        std::string datarefName = cmd.substr(prefix.size(), colonPos - prefix.size());
        float adjustment = std::stof(cmd.substr(colonPos + 1)) * steps;
        float current = Dataref::getInstance()->get<float>(datarefName.c_str());
        Dataref::getInstance()->set<float>(datarefName.c_str(), current + adjustment);
    } else {
        for (int i = 0; i < steps; i++) {
            Dataref::getInstance()->executeCommand(cmd.c_str());
        }
    }
}

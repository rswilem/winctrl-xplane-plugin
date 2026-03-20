#include "c172-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <XPLMUtilities.h>

C172FCUEfisProfile::C172FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 2) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t target = hasPower ? brightness[1] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, 0);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/physics/metric_press", [product](bool isMetric) {
        product->updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/servos_on", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, isAutopilotEngaged ? 1 : 0);
    });

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
}

C172FCUEfisProfile::~C172FCUEfisProfile() {
    Dataref::getInstance()->unbind("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/battery_on");
    Dataref::getInstance()->unbind("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    Dataref::getInstance()->unbind("sim/physics/metric_press");
    Dataref::getInstance()->unbind("sim/cockpit2/autopilot/servos_on");
}

bool C172FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/c172/electrical/battery_amps");
}

const std::vector<std::string> &C172FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &C172FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {3, {"AP", "sim/GPS/g1000n3_ap"}},

        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/GPS/g1000n3_hdg_sync"}}, // pressing on the heading knob
        {16, {"HDG PULL", "sim/GPS/g1000n3_hdg"}},

        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},

        {21, {"VS DEC", "sim/GPS/g1000n3_nose_down"}},
        {22, {"VS INC", "sim/GPS/g1000n3_nose_up"}},

        {25, {"ALT 100", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 100.0}},
        {26, {"ALT 1000", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 1000.0}},

        {41, {"BARO DEC", "sim/GPS/g1000n1_baro_down"}},
        {42, {"BARO INC", "sim/GPS/g1000n1_baro_up"}},

        {43, {"L_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},
    };

    return buttons;
}

void C172FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = false;
    data.displayTest = false;
    data.speed = "";
    data.heading = "";
    data.altitude = "";
    data.verticalSpeed = "";
    data.efisRight.baro = "";

    data.efisLeft.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    float baroPilot = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    data.efisLeft.setBaro(baroPilot, !datarefManager->getCached<bool>("sim/physics/metric_press"));
}

void C172FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        if (button->dataref == "custom_set_altitude_mode") {
            altitudeIncrements = button->value;
            return;
        }
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin) {
        if (button->dataref == "custom_altitude") {
            bool directionUp = button->value > 0.0f;
            std::string dataref = "sim/GPS/g1000n3_alt_" + std::string(altitudeIncrements >= 1000 ? "outer" : "inner") + "_" + (directionUp ? "up" : "down");
            datarefManager->executeCommand(dataref.c_str());
            return;
        }

        datarefManager->executeCommand(button->dataref.c_str());
    }
}

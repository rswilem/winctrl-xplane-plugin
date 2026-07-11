#include "c172-afl-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <XPLMUtilities.h>

C172AFLFCUEfisProfile::C172AFLFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // Panel/radio backlight follows the AirfoilLabs radio-stack rheostat, gated by its own breaker.
    Dataref::getInstance()->monitorExistingDataref<float>("C172/cockpit/lights/radioLt", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("C172/electric/bus2/instLtsBreaker/panelLights/rheoRadioLt/power");

        uint8_t target = hasPower ? brightness * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, 0);

        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/physics/metric_press", [product](bool isMetric) {
        product->updateDisplays();
    },
        this);

    // The KAP140 drives X-Plane's generic autopilot system (verified: its knob/heading
    // commands move sim/cockpit*/autopilot/*), so annunciators read from the generic datarefs.
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/servos_on", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, isAutopilotEngaged ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/nav_status", [product](int navStatus) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, navStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/approach_status", [product](int approachStatus) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, approachStatus > 0 ? 1 : 0);
    },
        this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("C172/cockpit/lights/radioLt");
}

bool C172AFLFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("C172/acfVariant");
}

const std::vector<std::string> &C172AFLFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "C172/cockpit/pilotAlt/baroPilot",
        "sim/physics/metric_press",
        "sim/cockpit/electrical/battery_on",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &C172AFLFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {1, {"LOC", "C172/cockpit/KAP140/nav"}},
        {3, {"AP", "C172/cockpit/KAP140/ap"}},
        {8, {"APPR", "C172/cockpit/KAP140/apr"}},

        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/heading_sync"}}, // pressing on the heading knob
        {16, {"HDG PULL", "C172/cockpit/KAP140/hdg"}},

        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},

        {21, {"VS DEC", "C172/cockpit/KAP140/dn"}},
        {22, {"VS INC", "C172/cockpit/KAP140/up"}},
        {23, {"VS PUSH", "C172/cockpit/KAP140/alt"}},
        {24, {"VS PULL", "C172/cockpit/KAP140/arm"}},

        {25, {"ALT 100", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 100.0}},
        {26, {"ALT 1000", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 1000.0}},

        {41, {"L_PRESS DEC", "C172/cockpit/pilotAlt/baroPilotDec"}},
        {42, {"L_PRESS INC", "C172/cockpit/pilotAlt/baroPilotInc"}},

        {43, {"L_inHg", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "sim/physics/metric_press", FCUEfisDatarefType::SET_VALUE, 1.0}},
    };

    return buttons;
}

void C172AFLFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = false;
    data.displayTest = false;
    data.speed = "";
    data.heading = "";
    data.altitude = "";
    data.verticalSpeed = "";
    data.efisRight.baro = "";

    data.efisLeft.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    float baroPilot = datarefManager->getCached<float>("C172/cockpit/pilotAlt/baroPilot");
    data.efisLeft.setBaro(baroPilot, !datarefManager->getCached<bool>("sim/physics/metric_press"));
}

void C172AFLFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
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
            // Inner ring = 100 ft steps, outer ring = 1000 ft steps (verified on the KAP140).
            std::string ring = altitudeIncrements >= 1000 ? "outer" : "inner";
            std::string dataref = "C172/cockpit/KAP140/knob/" + ring + (directionUp ? "Inc" : "Dec");
            datarefManager->executeCommand(dataref.c_str());
            return;
        }

        datarefManager->executeCommand(button->dataref.c_str());
    }
}

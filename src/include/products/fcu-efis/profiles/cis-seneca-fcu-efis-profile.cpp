#include "cis-seneca-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <XPLMUtilities.h>

CISSenecaFCUEfisProfile::CISSenecaFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
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
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, 0);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("CIS/PA34/instruments/altimeter/HpA", [product](bool isHpa) {
        product->updateDisplays();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("CIS/PA34/autopilot/ap_light", [product](bool isAutopilotEngaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, isAutopilotEngaged ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, isAutopilotEngaged ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/annunciators/flight_director", [product](bool enabled) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, enabled ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, enabled ? 1 : 0);
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
}

bool CISSenecaFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("CIS/PA34/timer_minutes");
}

const std::vector<std::string> &CISSenecaFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/autopilot/servos_on",
        "sim/cockpit/autopilot/heading_mag",
        "CIS/PA34/autopilot/nav_status",
        "CIS/PA34/autopilot/heading_status",
        "sim/cockpit2/autopilot/altitude_hold_status",
        "sim/cockpit/electrical/battery_on",
        "CIS/PA34/instruments/altimeter/HpA",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &CISSenecaFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {1, {"LOC", "sim/autopilot/approach"}},
        {3, {"AP", "sim/autopilot/servos_toggle"}},
        {8, {"APPR", "sim/autopilot/approach"}},

        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/NAV"}},
        {16, {"HDG PULL", "sim/autopilot/heading"}},

        //        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        //        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {19, {"ALT PUSH", "sim/autopilot/altitude_hold"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_hold"}},
        {21, {"VS DEC", "sim/autopilot/nose_down"}},
        {22, {"VS INC", "sim/autopilot/nose_up"}},
        {23, {"VS PUSH", "sim/autopilot/altitude_hold"}},
        {24, {"VS PULL", "sim/autopilot/altitude_hold"}},

        //        {25, {"ALT 100", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 100.0}},
        //        {26, {"ALT 1000", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 1000.0}},

        {32, {"L_FD", "sim/autopilot/fdir_servos_toggle"}},

        {41, {"BARO DEC", "sim/instruments/barometer_down"}},
        {42, {"BARO INC", "sim/instruments/barometer_up"}},
        {43, {"L_inHg", "CIS/PA34/instruments/altimeter/HpA", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "CIS/PA34/instruments/altimeter/HpA", FCUEfisDatarefType::SET_VALUE, 1.0}},

        {64, {"R_FD", "sim/autopilot/fdir_servos_toggle"}},
    };

    return buttons;
}

void CISSenecaFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto datarefManager = Dataref::getInstance();

    data.displayEnabled = datarefManager->getCached<bool>("sim/cockpit2/autopilot/servos_on");
    data.displayTest = false;
    data.displayEnabledWindowsFlag = FCUDisplayData::Window::None;

    if (datarefManager->getCached<bool>("CIS/PA34/autopilot/heading_status") || datarefManager->getCached<bool>("CIS/PA34/autopilot/nav_status")) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::HeadingTrackHeader;
        data.headingHdg = datarefManager->getCached<bool>("CIS/PA34/autopilot/heading_status");
        data.headingLat = datarefManager->getCached<bool>("CIS/PA34/autopilot/nav_status");
    }

    if (datarefManager->getCached<int>("sim/cockpit2/autopilot/altitude_hold_status") > 0) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::AltitudeHeader;
        data.altIndication = true;
    }

    data.efisRight.baro = "";
    data.efisLeft.displayEnabled = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    float baroPilot = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    data.efisLeft.setBaro(baroPilot, !datarefManager->getCached<bool>("CIS/PA34/instruments/altimeter/HpA"));
}

void CISSenecaFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), button->value);
    } else if (phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

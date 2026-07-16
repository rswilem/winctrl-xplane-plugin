#include "c172-afl-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <XPLMUtilities.h>

C172AFLFCUEfisProfile::C172AFLFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    // Backlight follows the AirfoilLabs panel-light rheostat, gated by its own breaker.
    Dataref::getInstance()->monitorExistingDataref<float>("C172/cockpit/lights/panelLt", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("C172/electric/bus2/instLtsBreaker/panelLights/rheoPanelLt/power");

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

    // The backlight also depends on the panel-light power gate, but that flips on independently
    // of panelLt (e.g. when the bus powers up after load). Re-fire the brightness callback when
    // it changes so the backlight isn't stuck at its power-off value until the knob is touched.
    Dataref::getInstance()->monitorExistingDataref<bool>("C172/electric/bus2/instLtsBreaker/panelLights/rheoPanelLt/power", [](bool hasPower) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("C172/cockpit/lights/panelLt");
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

    Dataref::getInstance()->executeChangedCallbacksForDataref("C172/cockpit/lights/panelLt");
}

bool C172AFLFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("C172/acfVariant");
}

const std::vector<std::string> &C172AFLFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "C172/cockpit/pilotAlt/baroPilot",
        "sim/physics/metric_press",
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit2/autopilot/servos_on",
        "sim/cockpit2/autopilot/heading_status",
        "sim/cockpit2/autopilot/nav_status",
        "sim/cockpit2/autopilot/approach_status",
        "sim/cockpit2/autopilot/altitude_hold_status",
        "sim/cockpit2/autopilot/vvi_status",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",
        "C172/electric/av2/autoPilotBreaker/kap140/power",
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
        // HDG PUSH (15) intentionally unmapped: the analog KAP140 C172 has no heading-knob-sync
        // function (it exists only on the G1000 variant), so pressing the knob does nothing here.
        {16, {"HDG PULL", "C172/cockpit/KAP140/hdg"}},

        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {19, {"ALT PUSH", "C172/cockpit/KAP140/alt"}}, // altitude hold at current altitude
        {20, {"ALT PULL", "C172/cockpit/KAP140/arm"}}, // arm the preselected altitude for capture

        {21, {"VS DEC", "C172/cockpit/KAP140/dn"}},
        {22, {"VS INC", "C172/cockpit/KAP140/up"}},
        // VS PUSH (23) / VS PULL (24) unmapped: the KAP140 has no V/S-knob push/pull function;
        // vertical speed is flown with the UP/DN buttons (21/22) once ALT hold is released.

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

    // The KAP140 display is alive whenever it is powered (avionics on), independent of whether the
    // autopilot is engaged. Mode headers below light per active mode; the selected altitude shows
    // whenever the display is on.
    data.displayEnabled = datarefManager->getCached<bool>("C172/electric/av2/autoPilotBreaker/kap140/power");
    data.displayTest = false;
    data.displayEnabledWindowsFlag = FCUDisplayData::Window::None;
    data.speed = "";
    data.heading = "";
    data.altitude = "";
    data.verticalSpeed = "";

    // Lateral: HDG label for heading mode, LAT label for NAV/APR tracking.
    bool hdgActive = datarefManager->getCached<int>("sim/cockpit2/autopilot/heading_status") > 0;
    bool navActive = datarefManager->getCached<int>("sim/cockpit2/autopilot/nav_status") > 0 ||
                     datarefManager->getCached<int>("sim/cockpit2/autopilot/approach_status") > 0;
    if (hdgActive || navActive) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::HeadingTrackHeader;
        data.headingHdg = hdgActive;
        data.headingLat = navActive;
    }

    // Vertical: ALT label for altitude hold, V/S label for vertical-speed mode.
    if (datarefManager->getCached<int>("sim/cockpit2/autopilot/altitude_hold_status") > 0) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::AltitudeHeader;
        data.altIndication = true;
    }

    // Selected altitude (KAP140 preselect) shown whenever the AP is engaged: 4 digits, expanding
    // to 5 only at/above 10000 ft. The window right-pads to width 5 with '0', so pre-pad the
    // 4-digit case with a leading space (renders as a blank digit) to keep it right-aligned.
    if (data.displayEnabled) {
        int selectedAltitude = static_cast<int>(datarefManager->getCached<float>("sim/cockpit/autopilot/altitude"));
        std::ostringstream altStream;
        if (selectedAltitude >= 10000) {
            altStream << selectedAltitude;
        } else {
            altStream << ' ' << std::setw(4) << std::setfill('0') << selectedAltitude;
        }
        data.altitude = altStream.str();
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::AltitudeValue;
    }

    // Vertical speed: the "V/S" label above the V/S window plus the signed fpm value, shown only
    // while VS mode is active. VerticalSpeedFPAHeader/vsIndication is the label (byte V0); NOT
    // HdgTrkVsFpaHeader, which would also light the center HDG/TRK/V/S/FPA cluster. The value goes
    // in as a 4-digit magnitude with vsSign controlling +/- (same rendering as the TolISS profile).
    if (datarefManager->getCached<int>("sim/cockpit2/autopilot/vvi_status") > 0) {
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::VerticalSpeedFPAHeader;
        data.displayEnabledWindowsFlag |= FCUDisplayData::Window::VerticalSpeedFPAValue;
        data.vsIndication = true;

        float verticalSpeed = datarefManager->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
        int absVerticalSpeed = std::abs(static_cast<int>(std::round(verticalSpeed)));
        std::ostringstream vsStream;
        vsStream << std::setw(4) << std::setfill('0') << absVerticalSpeed;
        data.verticalSpeed = vsStream.str();
        data.vsSign = (verticalSpeed >= 0);
        data.vsVerticalLine = true;
        data.fpaComma = false;
    }

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

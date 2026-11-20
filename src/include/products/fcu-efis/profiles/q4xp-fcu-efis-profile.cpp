#include "q4xp-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

Q4XPFCUEfisProfile::Q4XPFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("FJS/Q4XP/Lights/panelText_LIT", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 1) {
            return;
        }

        uint8_t target = static_cast<uint8_t>(std::clamp(brightness[1], 0.0f, 1.0f) * 255);
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->forceStateSync();
    },
        this);

    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, 128);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, 128);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, 128);
    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 255);
    product->forceStateSync();
}

Q4XPFCUEfisProfile::~Q4XPFCUEfisProfile() {
}

bool Q4XPFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("FJS/Q4XP/cdu1/text_line_0");
}

const std::vector<std::string> &Q4XPFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/autopilot/airspeed",
        "sim/cockpit/autopilot/heading_mag",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit/autopilot/vertical_velocity",
        "sim/cockpit/misc/barometer_setting",
        "sim/cockpit/misc/barometer_setting2",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &Q4XPFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU buttons
        {3,  {"AP1",  "FJS/Q4XP/Autopilot/FGCP_BUTTON_AP"}},
        {4,  {"AP2",  "FJS/Q4XP/Autopilot/FGCP_BUTTON_AP"}},
        {7,  {"METRIC", "FJS/Q4XP/Switches/PFDALTUNITS"}},

        // Speed encoder
        {9,  {"SPD DEC",  "sim/autopilot/airspeed_down"}},
        {10, {"SPD INC",  "sim/autopilot/airspeed_up"}},
        {11, {"SPD PUSH", "sim/autopilot/airspeed_sync"}},
        {12, {"SPD PULL", "sim/autopilot/airspeed_sync"}},

        // Heading encoder
        {13, {"HDG DEC",  "sim/autopilot/heading_down"}},
        {14, {"HDG INC",  "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/heading"}},
        {16, {"HDG PULL", "sim/autopilot/heading_sync"}},

        // Altitude encoder
        {17, {"ALT DEC",  "sim/autopilot/altitude_down"}},
        {18, {"ALT INC",  "sim/autopilot/altitude_up"}},
        {19, {"ALT PUSH", "sim/autopilot/altitude_arm"}},
        {20, {"ALT PULL", "sim/autopilot/altitude_arm"}},

        // Vertical Speed encoder
        {21, {"VS DEC",  "sim/autopilot/vertical_speed_down"}},
        {22, {"VS INC",  "sim/autopilot/vertical_speed_up"}},
        {23, {"VS PUSH", "sim/autopilot/vertical_speed_sync"}},
        {24, {"VS PULL", "sim/autopilot/vertical_speed_sync"}},

        // EFIS Left — Baro
        {39, {"L_STD PUSH", "sim/instruments/barometer_std"}},
        {40, {"L_STD PULL", "sim/instruments/barometer_std"}},
        {41, {"L_BARO DEC", "sim/instruments/barometer_down"}},
        {42, {"L_BARO INC", "sim/instruments/barometer_up"}},

        // EFIS Right — Baro
        {71, {"R_STD PUSH", "sim/instruments/barometer_copilot_std"}},
        {72, {"R_STD PULL", "sim/instruments/barometer_copilot_std"}},
        {73, {"R_BARO DEC", "sim/instruments/barometer_copilot_down"}},
        {74, {"R_BARO INC", "sim/instruments/barometer_copilot_up"}},
    };

    return buttons;
}

void Q4XPFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dm = Dataref::getInstance();

    data.displayEnabled = true;

    // Speed
    float ias = dm->getCached<float>("sim/cockpit/autopilot/airspeed");
    {
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << static_cast<int>(std::round(ias));
        data.speed = oss.str();
    }
    data.spdMach = false;
    data.spdManaged = false;

    // Heading
    float hdg = dm->getCached<float>("sim/cockpit/autopilot/heading_mag");
    {
        int hdgInt = static_cast<int>(std::round(hdg)) % 360;
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << hdgInt;
        data.heading = oss.str();
    }
    data.headingHdg = true;
    data.headingTrk = false;
    data.headingLat = false;
    data.hdgManaged = false;

    // Altitude
    float alt = dm->getCached<float>("sim/cockpit/autopilot/altitude");
    {
        std::ostringstream oss;
        oss << std::internal << std::setw(5) << std::setfill('0') << static_cast<int>(std::round(alt));
        data.altitude = oss.str();
    }
    data.altManaged = false;
    data.altIndication = true;
    data.lvlChange = false;

    // Vertical Speed
    float vs = dm->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    {
        bool negative = vs < 0;
        int vsAbs = static_cast<int>(std::round(std::abs(vs)));
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << vsAbs;
        data.verticalSpeed = oss.str();
        data.vsSign = !negative;
    }
    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    data.vsHorizontalLine = false;
    data.vsVerticalLine = false;

    // EFIS Baro — inHg → convert to hPa/mbar display
    float baro1 = dm->getCached<float>("sim/cockpit/misc/barometer_setting");
    data.efisLeft.displayEnabled = true;
    data.efisLeft.unitIsInHg = false;
    data.efisLeft.setBaro(baro1, false);

    float baro2 = dm->getCached<float>("sim/cockpit/misc/barometer_setting2");
    data.efisRight.displayEnabled = true;
    data.efisRight.unitIsInHg = false;
    data.efisRight.setBaro(baro2, false);
}

void Q4XPFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

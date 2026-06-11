#include "jf146-fcu-efis-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

JF146FCUEfisProfile::JF146FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/panel_brightness_ratio", [product](const std::vector<float> &brightness) {
        if (brightness.size() < 4) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t target = hasPower ? brightness[0] * 255 : 0;
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, hasPower ? 255 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, hasPower ? 255 : 0);

        uint8_t screenBrightness = hasPower ? 255 : 0;
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screenBrightness);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screenBrightness);

        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/panel_brightness_ratio");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/annunciators/autopilot", [product](int status) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, status == 1 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("thranda/TMS/tmsPwr", [product](int status) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, status == 1 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("thranda/autopilot/FD_Show_Pilot", [product](int status) {
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, status == 1 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit2/autopilot/glideslope_status", [product](int status) {
        product->setLedBrightness(FCUEfisLed::APPR_GREEN, status > 0 ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("thranda/TCAS/AnnLtA", [product](const std::vector<float> &lights) {
        // Ensure cache is updated and trigger display update if needed
        product->forceStateSync();
    }, this);
}

// destructor, unbinds stuff it used so remember to put the ones you use in here
JF146FCUEfisProfile::~JF146FCUEfisProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool JF146FCUEfisProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");

    // Will only match the 146-100, 200, 300, not the Avro
    return icao.starts_with("B46");
}

const std::vector<std::string> &JF146FCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit/autopilot/altitude",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot",
        "sim/cockpit/autopilot/heading_mag",
        "thranda/autopilot/FD_Show_Pilot",
        "sim/cockpit2/gauges/indicators/airspeed_kts_pilot",
        "sim/cockpit2/gauges/indicators/mach_pilot",
        "sim/cockpit/autopilot/airspeed_is_mach",
        "thranda/TCAS/AnnLtA",
        "sim/cockpit2/autopilot/glideslope_status"};

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &JF146FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"SPD/MACH", "sim/autopilot/knots_mach_toggle"}},
        {1, {"LOC", "thranda/buttons/Button09"}}, // B LOC
        {2, {"TRK", "sim/autopilot/trkfpa"}},     // unused?
        {3, {"AP1", "sim/autopilot/servos_toggle"}},
        {4, {"AP2", ""}},                                // unused?
        {5, {"A/THR", "thranda/TMS/tmsPwrCmd"}},         // TMS on
        {6, {"EXPED", ""}},                              // unused - TMS?
        {7, {"METRIC", ""}},                             // unused
        {8, {"APPR", "thranda/buttons/Button07"}},       // GSL button
        {9, {"SPD DEC", "thranda/knob/RheostatDn104"}},  // SPD changed with SYNC button, so this is just for the bug (controls fast/slow)
        {10, {"SPD INC", "thranda/knob/RheostatUp104"}}, // "
        {11, {"SPD PUSH", "thranda/buttons/Button16"}},  // IAS
        {12, {"SPD PULL", "thranda/buttons/Button12"}},  // MACH
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "thranda/buttons/Button14"}}, // LNAV
        {16, {"HDG PULL", "thranda/buttons/Button15"}}, // HDG
        {17, {"ALT DEC", "thranda/knob/RheostatDn15"}},
        {18, {"ALT INC", "thranda/knob/RheostatUp15"}},
        {19, {"ALT PUSH", "sim/autopilot/altitude_arm"}},       // ALT ARM
        {20, {"ALT PULL", "thranda/buttons/Button08"}},         // ALT mode
        {21, {"VS DEC", "sim/autopilot/nose_down_pitch_mode"}}, // pitch down AP
        {22, {"VS INC", "sim/autopilot/nose_up_pitch_mode"}},
        {23, {"VS PUSH", "thranda/buttons/Button13"}}, // VNAV
        {24, {"VS PULL", "thranda/buttons/Button11"}}, // VS
        {25, {"ALT 100", ""}},                         // handle in code?
        {26, {
                 "ALT 1000",
                 "",
             }},

        // Buttons 27-31 reserved

        {32, {"L_FD", "custom"}},                   // handle in code? Switch 92
        {33, {"L_LS", "thranda/buttons/Button06"}}, // V/L button
        {34, {"L_CSTR", "thranda/TMS/toGaMode"}},   // TO mode
        {35, {"L_WPT", "thranda/TMS/mctMode"}},     // MCT mode
        {36, {"L_VOR.D", "thranda/TMS/tgtMode"}},   // TGT mode
        {37, {"L_NDB", "thranda/TMS/descMode"}},    // DESC mode
        {38, {"L_ARPT", "thranda/TMS/tmsSync"}},    // SYNC mode
        {39, {"L_STD PUSH", "sim/instruments/barometer_2992"}},
        {40, {"L_STD PULL", ""}},
        // {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        // {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {41, {"L_PRESS DEC", "thranda/knob/RheostatDn105"}},
        {42, {"L_PRESS INC", "thranda/knob/RheostatUp105"}},
        {43, {"L_inHg", ""}},
        {44, {"L_hPa", ""}},
        {45, {"L_MODE LS", ""}},
        {46, {"L_MODE VOR", ""}},
        {47, {"L_MODE NAV", ""}},
        {48, {"L_MODE ARC", ""}},
        {49, {"L_MODE PLAN", ""}},
        {50, {"L_RANGE 10", ""}},
        {51, {"L_RANGE 20", ""}},
        {52, {"L_RANGE 40", ""}},
        {53, {"L_RANGE 80", ""}},
        {54, {"L_RANGE 160", ""}},
        {55, {"L_RANGE 320", ""}},
        {56, {"L_1 ADF", ""}},
        {57, {"L_1 OFF", ""}},
        {58, {"L_1 VOR", ""}},
        {59, {"L_2 ADF", ""}},
        {60, {"L_2 OFF", ""}},
        {61, {"L_2 VOR", ""}},
        // Buttons 62-63 reserved

        {64, {"R_FD", "custom"}},                   // handle in code?
        {65, {"R_LS", "thranda/buttons/Button06"}}, // V/L button
        {66, {"R_CSTR", ""}},
        {67, {"R_WPT", ""}},
        {68, {"R_VOR.D", ""}},
        {69, {"R_NDB", ""}},
        {70, {"R_ARPT", ""}},
        {71, {"R_STD PUSH", "sim/instruments/barometer_2992"}},
        {72, {"R_STD PULL", ""}},
        {73, {"R_PRESS DEC", "thranda/knob/RheostatDn116"}},
        {74, {"R_PRESS INC", "thranda/knob/RheostatUp116"}},
        {75, {"R_inHg", ""}},
        {76, {"R_hPa", ""}},
        {77, {"R_MODE LS", ""}},
        {78, {"R_MODE VOR", ""}},
        {79, {"R_MODE NAV", ""}},
        {80, {"R_MODE ARC", ""}},
        {81, {"R_MODE PLAN", ""}},
        {82, {"R_RANGE 10", ""}},
        {83, {"R_RANGE 20", ""}},
        {84, {"R_RANGE 40", ""}},
        {85, {"R_RANGE 80", ""}},
        {86, {"R_RANGE 160", ""}},
        {87, {"R_RANGE 320", ""}},
        {88, {"R_1 VOR", ""}},
        {89, {"R_1 OFF", ""}},
        {90, {"R_1 ADF", ""}},
        {91, {"R_2 VOR", ""}},
        {92, {"R_2 OFF", ""}},
        {93, {"R_2 ADF", ""}},
        // Buttons 94-95 reserved
    };
    return buttons;
}

void JF146FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/battery_on");

    if (!hasPower) {
        // Clear all displays when no power
        data.speed = {"", {}};
        data.heading = {"", {}};
        data.altitude = {"", {}};
        data.verticalSpeed = {"", {}};
        data.efisRight.baro = "";
        data.efisLeft.baro = "";
        return;
    }

    data.headingHdg = true;
    data.headingLat = true;

    // Speed display
    bool isMach = Dataref::getInstance()->getCached<int>("sim/cockpit/autopilot/airspeed_is_mach");

    if (isMach) {
        float speed = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/indicators/mach_pilot");
        // Display as Mach number (e.g., "0.78" -> ".78")
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << speed;
        std::string machStr = ss.str();
        if (machStr[0] == '0') {
            machStr = machStr.substr(1); // Remove leading 0
        }
        data.speed = {machStr, {}};
        data.spdMach = true;

    } else {
        float speed = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/indicators/airspeed_kts_pilot");
        // Display as knots
        std::ostringstream ss;
        ss << static_cast<int>(speed);
        data.speed = ss.str();
        data.spdMach = false;
    }

    // Heading display
    float heading = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/heading_mag");
    std::ostringstream headingSs;
    headingSs << std::setw(3) << std::setfill('0') << static_cast<int>(heading);
    data.heading = headingSs.str();

    // Altitude display
    float altitude = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/altitude");
    std::ostringstream altSs;
    altSs << std::setw(5) << std::setfill('0') << static_cast<int>(altitude);
    data.altitude = altSs.str();

    // Vertical speed display
    float vs = Dataref::getInstance()->getCached<float>("sim/cockpit/autopilot/vertical_velocity");
    std::ostringstream vsSs;
    int vsInt = static_cast<int>(vs);
    int absVs = std::abs(vsInt);
    vsSs << std::setw(4) << std::setfill('0') << absVs;
    data.verticalSpeed = vsSs.str();
    data.vsSign = (vs >= 0);
    data.vsMode = true;

    // Barometer displays (inHg)
    float baroPilot = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    float baroFO = Dataref::getInstance()->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_copilot");

    data.efisLeft.setBaro(baroPilot, true);
    data.efisRight.setBaro(baroFO, true);

    // Update Right EFIS LEDs from TCAS Annunciator array
    std::vector<float> tcasLights = Dataref::getInstance()->getCached<std::vector<float>>("thranda/TCAS/AnnLtA");

    if (tcasLights.size() >= 376) {
        auto product = dynamic_cast<ProductFCUEfis *>(this->product);
        if (product) {
            // Right EFIS
            // ARPT: 362 || 363
            product->setLedBrightness(FCUEfisLed::EFISR_ARPT_GREEN, (tcasLights[362] > 0.1f || tcasLights[363] > 0.1f) ? 255 : 0);
            // NDB: 364 || 365
            product->setLedBrightness(FCUEfisLed::EFISR_NDB_GREEN, (tcasLights[364] > 0.1f || tcasLights[365] > 0.1f) ? 255 : 0);
            // VOR.D: 366 || 367
            product->setLedBrightness(FCUEfisLed::EFISR_VORD_GREEN, (tcasLights[366] > 0.1f || tcasLights[367] > 0.1f) ? 255 : 0);
            // WPT: 368 || 369
            product->setLedBrightness(FCUEfisLed::EFISR_WPT_GREEN, (tcasLights[368] > 0.1f || tcasLights[369] > 0.1f) ? 255 : 0);

            // Left EFIS
            // CSTR (TO): 370
            product->setLedBrightness(FCUEfisLed::EFISL_CSTR_GREEN, (tcasLights[370] > 0.1f) ? 255 : 0);
            // WPT (MCT): 372
            product->setLedBrightness(FCUEfisLed::EFISL_WPT_GREEN, (tcasLights[372] > 0.1f) ? 255 : 0);
            // VOR.D (TGT): 373
            product->setLedBrightness(FCUEfisLed::EFISL_VORD_GREEN, (tcasLights[373] > 0.1f) ? 255 : 0);
            // NDB (DESC): 374
            product->setLedBrightness(FCUEfisLed::EFISL_NDB_GREEN, (tcasLights[374] > 0.1f) ? 255 : 0);
            // ARPT (SYNC): 375
            product->setLedBrightness(FCUEfisLed::EFISL_ARPT_GREEN, (tcasLights[375] > 0.1f) ? 255 : 0);
        }
    }
}

void JF146FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    // Debounce logic
    if (phase == xplm_CommandBegin) {
        float now = XPLMGetElapsedTime();
        if (lastPressTime.find(button) != lastPressTime.end()) {
            if (now - lastPressTime[button] < 0.25f) {
                return;
            }
        }
        lastPressTime[button] = now;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->name == "L_FD" && phase == xplm_CommandBegin) {
        int isOn = Dataref::getInstance()->get<int>("thranda/autopilot/FD_Show_Pilot");

        isOn ? datarefManager->executeCommand("thranda/switches/SwitchDn92") : datarefManager->executeCommand("thranda/switches/SwitchUp92");

    } else if (button->name == "R_FD" && phase == xplm_CommandBegin) {
        int isOn = Dataref::getInstance()->get<int>("thranda/autopilot/FD_Show_CoPilot");

        isOn ? datarefManager->executeCommand("thranda/switches/SwitchDn76") : datarefManager->executeCommand("thranda/switches/SwitchUp76");

    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

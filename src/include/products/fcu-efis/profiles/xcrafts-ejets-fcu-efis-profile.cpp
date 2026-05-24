#include "xcrafts-ejets-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

XCraftsEjetsFCUEfisProfile::XCraftsEjetsFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("XCrafts/panel_brt_1", [product](float brightness) {
        uint8_t target = static_cast<uint8_t>(brightness * 255);
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, target);
        product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, target);

        product->forceStateSync();
    });

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit/autopilot/autopilot_mode", [this, product](int mode) {
        bool engaged = (mode == 2);
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, (engaged || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::AP2_GREEN, (engaged || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISL_FD_GREEN, (engaged || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FCUEfisLed::EFISR_FD_GREEN, (engaged || isAnnunTest()) ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/autothrottle_armed", [this, product](int armed) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, (armed == 1 || isAnnunTest()) ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/autopilot/autothrottle_system_active", [this, product](int active) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, (active == 1 || isAnnunTest()) ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("XCrafts/ERJ/cockpit/annunciators_test", [this](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/autopilot/autopilot_mode");
        Dataref::getInstance()->executeChangedCallbacksForDataref("XCrafts/ERJ/autothrottle_armed");
        Dataref::getInstance()->executeChangedCallbacksForDataref("XCrafts/ERJ/autopilot/autothrottle_system_active");
    });
}

XCraftsEjetsFCUEfisProfile::~XCraftsEjetsFCUEfisProfile() {
    Dataref::getInstance()->unbind("XCrafts/panel_brt_1");
    Dataref::getInstance()->unbind("sim/cockpit/autopilot/autopilot_mode");
    Dataref::getInstance()->unbind("XCrafts/ERJ/autothrottle_armed");
    Dataref::getInstance()->unbind("XCrafts/ERJ/autopilot/autothrottle_system_active");
    Dataref::getInstance()->unbind("XCrafts/ERJ/cockpit/annunciators_test");
}

bool XCraftsEjetsFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/FMS/CDU_1_01");
}

const std::vector<std::string> &XCraftsEjetsFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "XCrafts/ERJ/autopilot/airspeed_dial_kts_mach",
        "sim/cockpit/autopilot/airspeed_is_mach",

        "sim/cockpit/autopilot/heading_mag",

        "XCrafts/ERJ/autopilot/altitude",

        "XCrafts/ERJ/autopilot/vertical_velocity",

        "sim/cockpit/autopilot/autopilot_mode",
        "XCrafts/ERJ/autothrottle_armed",
        "XCrafts/ERJ/autopilot/autothrottle_system_active",
        "XCrafts/speed_knob_fms_man",

        "XCrafts/ERJ/cockpit/annunciators_test",
        "sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot",
        "sim/physics/metric_press",
        "XCrafts/ERJ/STD_visible",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &XCraftsEjetsFCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", "XCrafts/ERJ/MACH_KIAS_toggle"}},
        {1, {"LOC", "XCrafts/ERJ/LNAV"}},
        {2, {"TRK/FPA", "XCrafts/ERJ/FPA"}},
        {3, {"AP1", "XCrafts/APYD_Toggle"}},
        {4, {"AP2", "XCrafts/APYD_Toggle"}},
        {5, {"A/THR", "XCrafts/ERJ/AutoThrottle"}},
        {7, {"METRIC", "XCrafts/ERJ/PFD/altitude_meters"}},
        {8, {"APPR", "XCrafts/ERJ/APPCH"}},
        {9, {"SPD DEC", "custom_airspeed", FCUEfisDatarefType::ADJUST_VALUE, -1.0}},
        {10, {"SPD INC", "custom_airspeed", FCUEfisDatarefType::ADJUST_VALUE, 1.0}},
        {11, {"SPD PUSH", "XCrafts/ERJ/PFD/AT_speed_source"}}, // or set XCrafts/speed_knob_fms_man to 0
        {12, {"SPD PULL", "XCrafts/ERJ/PFD/AT_speed_source"}}, // or set XCrafts/speed_knob_fms_man to 1
        {13, {"HDG DEC", "sim/autopilot/heading_down"}},
        {14, {"HDG INC", "sim/autopilot/heading_up"}},
        {15, {"HDG PUSH", "sim/autopilot/heading_sync"}},
        {16, {"HDG PULL", "sim/autopilot/heading"}},
        {17, {"ALT DEC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, -1.0}},
        {18, {"ALT INC", "custom_altitude", FCUEfisDatarefType::EXECUTE_CMD_ONCE, 1.0}},
        {19, {"ALT PUSH", "XCrafts/ERJ/VNAV"}},
        {20, {"ALT PULL", "XCrafts/ERJ/FLCH"}},
        {21, {"VS DEC", "XCrafts/ERJ/autopilot/vertical_velocity", FCUEfisDatarefType::ADJUST_VALUE, -100.0}},
        {22, {"VS INC", "XCrafts/ERJ/autopilot/vertical_velocity", FCUEfisDatarefType::ADJUST_VALUE, 100.0}},
        {23, {"VS PUSH", "XCrafts/ERJ/alt_hold"}},
        {24, {"VS PULL", "XCrafts/ERJ/VS"}},
        {25, {"ALT 100", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 100.0}},
        {26, {"ALT 1000", "custom_set_altitude_mode", FCUEfisDatarefType::SET_VALUE, 1000.0}},

        // Buttons 27-31 reserved

        {32, {"L_FD", "XCrafts/ERJ/fdir_toggle"}},
        {33, {"L_LS", "XCrafts/PFD/PREV"}},
        {39, {"L_STD", "XCrafts/ERJ/STD"}},
        {40, {"L_STD PULL", "XCrafts/ERJ/STD"}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},

        {45, {"L_MODE LS", "XCrafts/ERJ/MFD/MAP_tab"}},
        {46, {"L_MODE VOR", "XCrafts/ERJ/MFD/MAP_tab"}},
        {47, {"L_MODE NAV", "XCrafts/ERJ/MFD/SYS_tab"}},
        {48, {"L_MODE ARC", "XCrafts/ERJ/MFD/MAP_tab"}},
        {49, {"L_MODE PLAN", "XCrafts/ERJ/MFD/PLAN_tab"}},
        {50, {"L_RANGE 10", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "sim/cockpit/switches/EFIS_map_range_selector", FCUEfisDatarefType::SET_VALUE, 5.0}},
        // Buttons 62-63 reserved

        {64, {"R_FD", "XCrafts/ERJ/fdir_toggle"}},
        {65, {"R_LS", "XCrafts/PFD/PREV"}},
        {71, {"R_STD", "XCrafts/ERJ/STD"}},
        {72, {"R_STD PULL", "XCrafts/ERJ/STD"}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        // Buttons 94-95 reserved
    };

    return buttons;
}

void XCraftsEjetsFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    auto dr = Dataref::getInstance();

    data.displayEnabled = true;
    data.displayTest = dr->getCached<bool>("XCrafts/ERJ/cockpit/annunciators_test");
    data.displayEnabledWindowsFlag &= ~FCUDisplayData::Window::LevelChangeHeader;

    // Speed / Mach
    data.spdMach = dr->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
    float speed = dr->getCached<float>("XCrafts/ERJ/autopilot/airspeed_dial_kts_mach");

    if (speed > 0) {
        std::stringstream ss;
        if (data.spdMach) {
            int machHundredths = static_cast<int>(std::round(speed * 100));
            ss << std::setfill('0') << std::setw(3) << machHundredths;
        } else {
            ss << std::setfill('0') << std::setw(3) << static_cast<int>(speed);
        }
        data.speed = ss.str();
    } else {
        data.speed = "---";
    }

    // Heading
    float heading = dr->getCached<float>("sim/cockpit/autopilot/heading_mag");
    if (heading >= 0) {
        int hdgDisplay = static_cast<int>(heading) % 360;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << hdgDisplay;
        data.heading = ss.str();
    } else {
        data.heading = "---";
    }

    // HDG/TRK and VS/FPA mode — no readable state dataref provided, default to HDG/VS
    data.headingHdg = true;
    data.headingTrk = false;
    data.vsMode = true;
    data.fpaMode = false;
    data.headingLat = false;

    // Altitude
    float altitude = dr->getCached<float>("XCrafts/ERJ/autopilot/altitude"); // ugh.. 11900 (rounded FL110 maar display zegt FL111)
    if (altitude >= 0) {
        int altInt = (static_cast<int>(altitude) / 100) * 100;
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << altInt;
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    // Vertical speed
    float vs = dr->getCached<float>("XCrafts/ERJ/autopilot/vertical_velocity");
    int vsInt = (static_cast<int>(vs) / 100) * 100;
    int absVs = std::abs(vsInt);

    std::stringstream vsSS;
    if (absVs % 100 == 0) {
        vsSS << std::setfill('0') << std::setw(2) << (absVs / 100) << "##";
    } else {
        vsSS << std::setfill('0') << std::setw(4) << absVs;
    }
    data.verticalSpeed = vsSS.str();
    data.vsSign = (vs >= 0);
    data.fpaComma = false;

    data.vsIndication = data.vsMode;
    data.fpaIndication = false;
    data.vsVerticalLine = data.vsMode && (data.verticalSpeed != "-----");

    data.spdManaged = Dataref::getInstance()->getCached<int>("XCrafts/speed_knob_fms_man") == 1;
    data.hdgManaged = false;
    data.altManaged = false;

    // EFIS baro display
    float baroInHg = dr->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
    bool isMetric = dr->getCached<bool>("sim/physics/metric_press");

    EfisDisplayValue efisValue = {
        .displayEnabled = data.displayEnabled,
        .displayTest = data.displayTest,
        .baro = "",
        .unitIsInHg = !isMetric,
        .isStd = dr->getCached<bool>("XCrafts/ERJ/STD_visible"),
    };

    if (baroInHg > 0 && !efisValue.isStd) {
        efisValue.setBaro(baroInHg, !isMetric);
    }

    data.efisLeft = efisValue;
    data.efisRight = {.displayEnabled = false};
}

bool XCraftsEjetsFCUEfisProfile::isAnnunTest() {
    return Dataref::getInstance()->get<bool>("XCrafts/ERJ/cockpit/annunciators_test");
}

void XCraftsEjetsFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin && button->dataref == "custom_set_altitude_mode") {
        altitudeIncrements = button->value;
    } else if (phase == xplm_CommandBegin && button->dataref == "custom_altitude") {
        bool directionUp = button->value > 0.0f;
        float current = datarefManager->get<float>("XCrafts/ERJ/autopilot/altitude");
        datarefManager->set<float>("XCrafts/ERJ/autopilot/altitude", current + (directionUp ? altitudeIncrements : -altitudeIncrements));
    } else if (phase == xplm_CommandBegin && button->dataref == "custom_airspeed") {
        bool isMach = datarefManager->getCached<bool>("sim/cockpit/autopilot/airspeed_is_mach");
        float increment = isMach ? 0.01f : 1.0f;
        float current = datarefManager->get<float>("XCrafts/ERJ/autopilot/airspeed_dial_kts_mach");
        datarefManager->set<float>("XCrafts/ERJ/autopilot/airspeed_dial_kts_mach", current + (increment * button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::ADJUST_VALUE) {
        float current = datarefManager->get<float>(button->dataref.c_str());
        datarefManager->set<float>(button->dataref.c_str(), current + static_cast<float>(button->value));
    } else if (phase == xplm_CommandBegin && (button->datarefType == FCUEfisDatarefType::BAROMETER_PILOT || button->datarefType == FCUEfisDatarefType::BAROMETER_FO)) {
        bool isBaroHpa = datarefManager->getCached<bool>("sim/physics/metric_press");
        float baroValue = datarefManager->getCached<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot");
        bool increase = button->value > 0;

        if (isBaroHpa) {
            float hpaValue = baroValue * 33.8639f;
            hpaValue += increase ? 1.0f : -1.0f;
            baroValue = hpaValue / 33.8639f;
        } else {
            baroValue += increase ? 0.01f : -0.01f;
        }

        datarefManager->set<float>("sim/cockpit2/gauges/actuators/barometer_setting_in_hg_pilot", baroValue);

    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::SET_VALUE) {
        datarefManager->set<float>(button->dataref.c_str(), static_cast<float>(button->value));

    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::TOGGLE_VALUE) {
        int current = datarefManager->get<int>(button->dataref.c_str());
        datarefManager->set<int>(button->dataref.c_str(), current ? 0 : 1);

    } else if (phase == xplm_CommandBegin && button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

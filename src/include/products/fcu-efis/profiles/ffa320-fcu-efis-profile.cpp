#include "ffa320-fcu-efis-profile.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "ffsharedvalues.h"
#include "logger.hpp"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

// The A320 ultimate keeps its FCU behind FlightFactor's SharedValuesInterface,
// like the MCDU: only the cockpit click objects reach X-Plane as datarefs and
// commands. Aircraft.FMGS.FCU1 carries the logical FCU state and
// Aircraft.Cockpit.Panel.FCU_* / EFIS_* the panel objects; both trees are
// listed verbatim in the aircraft's own DOCs/publishable.txt.
//
// Two conventions in here are taken from the panel rather than from
// documentation, because the aircraft does not describe them anywhere:
//  - FCU1.Mode is 0 for HDG/VS and 1 for TRK/FPA.
//  - Rotary detents count up from zero in the order the real selector reads,
//    so ND mode is LS..PLAN and ND range 10..320.
// Both live in one place each so a single edit corrects them.

FFA320FCUEfisProfile::FFA320FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    displayVersionRef = std::string(PRODUCT_NAME "/ffa320/fcu/display_version");

    Dataref::getInstance()->createDataref<int>(displayVersionRef.c_str(), &displayVersion);

    connectToAircraft();

    // A maintained switch already in position reports nothing when this profile
    // replaces another, so the held detents are replayed.
    product->forceStateSync();
}

FFA320FCUEfisProfile::~FFA320FCUEfisProfile() {
    FFSharedValues::getInstance()->removeUpdateCallback(this);

    if (retained) {
        FFSharedValues::getInstance()->release();
        retained = false;
    }

    // The accessor points at our displayVersion member, so it cannot outlive us.
    Dataref::getInstance()->unbind(displayVersionRef.c_str());
}

bool FFA320FCUEfisProfile::IsEligible() {
    const std::string author = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_author");
    const std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");

    return author.starts_with("FlightFactor") && icao == "A320";
}

void FFA320FCUEfisProfile::connectToAircraft() {
    if (retained) {
        return;
    }

    auto shared = FFSharedValues::getInstance();

    if (!shared->retain(FFSharedValues::A320UltimateSignature)) {
        // The aircraft answers the interface request only once its systems
        // plugin is fully up, which is later than our profile match.
        AppState::getInstance()->executeAfter(2000, this, [this]() {
            connectToAircraft();
        });
        return;
    }

    retained = true;

    shared->addUpdateCallback(this, [this](double step) {
        pollAircraft();
    });
}

double FFA320FCUEfisProfile::panel(const std::string &leaf) const {
    return FFSharedValues::getInstance()->getNumber(PanelPath + leaf);
}

double FFA320FCUEfisProfile::fcu(const std::string &leaf) const {
    return FFSharedValues::getInstance()->getNumber(fcuPath + leaf);
}

void FFA320FCUEfisProfile::pollAircraft() {
    auto shared = FFSharedValues::getInstance();
    if (!shared->isConnected()) {
        return;
    }

    // FCU2 is the standby channel and carries the same leaves, so a missing
    // FCU1 is a channel choice rather than a dead end.
    if (shared->idForPath(fcuPath + "Altitude") < 0 && shared->idForPath("Aircraft.FMGS.FCU2.Altitude") >= 0) {
        fcuPath = "Aircraft.FMGS.FCU2.";
    }

    if (!loggedMissingTree && shared->idForPath(fcuPath + "Altitude") < 0) {
        loggedMissingTree = true;
        Logger::getInstance()->warn("FF A320: shared value \"%sAltitude\" did not resolve, the FCU display will stay blank.\n", fcuPath.c_str());
    }

    FCUDisplayData previous = snapshot;
    buildSnapshot();

    if (!(snapshot == previous) || !(snapshot.efisLeft == previous.efisLeft) || !(snapshot.efisRight == previous.efisRight)) {
        displayVersion++;
    }

    updateLeds();
    updateBrightness();
    syncAltitudeStep();
    releaseModeSwitches();
}

void FFA320FCUEfisProfile::buildSnapshot() {
    FCUDisplayData data;

    // Nothing here says cleanly whether the FCU is lit. The glyph emissive is
    // night-only and PowerLoad.Conduction is a current reading 0.019 on a live
    // panel, so any threshold on it blanks the windows the moment it dips. The
    // windows follow the tree: lit while the FCU answers, dark when it stops.
    conduction = fcu("PowerLoad.Conduction");
    data.displayEnabled = resolves(fcuPath + "Altitude");
    data.displayTest = false;

    const bool trkMode = fcu("Mode") > 0.5;
    data.headingTrk = trkMode;
    data.headingHdg = !trkMode;
    data.headingLat = true;
    data.vsMode = !trkMode;
    data.fpaMode = trkMode;

    // *Managed dashes the window; the dot also follows *Mode, the knob's managed
    // state. A turn while managed shows the preset digits with the dot still lit.
    const bool spdDashed = fcu("SpeedManaged") > 0.5;
    const bool hdgDashed = fcu("LateralManaged") > 0.5;
    data.spdManaged = spdDashed || fcu("SpeedMode") > 0.5;
    data.hdgManaged = hdgDashed || fcu("LateralMode") > 0.5;
    data.altManaged = fcu("AltitudeManaged") > 0.5 || fcu("AltitudeMode") > 0.5;

    data.spdMach = fcu("SpeedIsMach") > 0.5;

    // Speed is the window value, the way Lateral and Altitude are for theirs.
    // SpeedKnots is the aircraft's own speed, which is why the window read 188
    // while 304 was selected, and sat at the FCU floor of 100 when stopped.
    double speed = fcu("Speed");

    // Mach comes back in hundredths, as the window reads it.
    if (data.spdMach && speed > 1.5) {
        speed /= 100.0;
    }

    // Managed speed dashes the window, the target being the FMGC's to show.
    if (spdDashed || speed <= 0.0) {
        data.speed = "---";
    } else {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << static_cast<int>(std::round(data.spdMach ? speed * 100.0 : speed));
        data.speed = ss.str();
    }

    const double lateral = fcu("Lateral");
    if (hdgDashed || lateral < 0.0) {
        data.heading = "---";
    } else {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3) << (static_cast<int>(std::round(lateral)) % 360);
        data.heading = ss.str();
    }

    // The FCU counts altitude in hundreds of feet, its own smallest step, so a
    // fresh panel parked at 100 ft reads 1.
    const double altitude = fcu("Altitude") * 100.0;
    if (altitude >= 0.0) {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(5) << static_cast<int>(std::round(altitude));
        data.altitude = ss.str();
    } else {
        data.altitude = "-----";
    }

    // The vertical window is dashed whenever no V/S or FPA target is selected,
    // which is exactly what VerticalManaged reports.
    const bool verticalDashed = fcu("VerticalManaged") > 0.5;

    if (verticalDashed) {
        data.verticalSpeed = "-----";
        data.vsSign = false;
        data.fpaComma = data.fpaMode;
    } else if (data.fpaMode) {
        const double fpa = fcu("VerticalAngle");
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << static_cast<int>(std::round(std::abs(fpa) * 10)) << "  ";
        data.verticalSpeed = ss.str();
        data.fpaComma = true;
        data.vsSign = fpa >= 0.0;
    } else {
        // Vertical is the window value; VerticalSpeed is the aircraft's own
        // rate, which is why the window wandered with the climb.
        double vs = fcu("Vertical");

        // V/S is selectable only in hundreds, so a magnitude under 100 is the
        // aircraft counting in hundreds rather than a 60 ft/min target.
        if (vs != 0.0 && std::abs(vs) < 100.0) {
            vs *= 100.0;
        }

        const int absVs = std::abs(static_cast<int>(std::round(vs)));
        std::stringstream ss;
        if (absVs % 100 == 0) {
            ss << std::setfill('0') << std::setw(2) << (absVs / 100) << "##";
        } else {
            ss << std::setfill('0') << std::setw(4) << absVs;
        }
        data.verticalSpeed = ss.str();
        data.fpaComma = false;
        data.vsSign = vs >= 0.0;
    }

    data.vsIndication = data.vsMode;
    data.fpaIndication = data.fpaMode;
    data.vsVerticalLine = data.vsMode && data.verticalSpeed != "-----";

    for (int side = 0; side < 2; ++side) {
        const bool captain = side == 0;
        const char *suffix = captain ? "BaroL" : "BaroR";

        const double raw = fcu(suffix);
        // inHg arrives in hundredths (2992) and hPa whole (1013), two ranges
        // that cannot overlap, so the magnitude alone names the unit.
        const bool inHg = raw > 1500.0;
        const double baro = inHg ? raw / 100.0 : raw;

        EfisDisplayValue value = {
            .displayEnabled = data.displayEnabled,
            .displayTest = false,
            .baro = "",
            .unitIsInHg = inHg,
            .isStd = fcu(captain ? "BaroModeL" : "BaroModeR") > 0.5,
        };

        if (!value.isStd && baro > 0.0) {
            value.setBaro(inHg ? static_cast<float>(baro) : static_cast<float>(baro / 33.8639), inHg);
        }

        if (captain) {
            data.efisLeft = value;
        } else {
            data.efisRight = value;
        }
    }

    if (!loggedRawValues && data.displayEnabled) {
        loggedRawValues = true;
        Logger::getInstance()->info("FF A320 FCU speed leaves: Speed=%f Select=%f Knots=%f Mach=%f managed=%f isMach=%f\n",
            fcu("Speed"), fcu("SpeedSelect"), fcu("SpeedKnots"), fcu("SpeedMach"), fcu("SpeedManaged"), fcu("SpeedIsMach"));
        Logger::getInstance()->info("FF A320 FCU raw: mode=%f spd=%f/%f isMach=%f lat=%f alt=%f vs=%f fpa=%f baroL=%f baroR=%f\n",
            fcu("Mode"), fcu("SpeedKnots"), fcu("SpeedMach"), fcu("SpeedIsMach"), fcu("Lateral"),
            fcu("Altitude"), fcu("Vertical"), fcu("VerticalAngle"), fcu("BaroL"), fcu("BaroR"));
        Logger::getInstance()->info("FF A320 FCU vertical leaves: Vertical=%f Select=%f Speed=%f Angle=%f managed=%f type=%f\n",
            fcu("Vertical"), fcu("VerticalSelect"), fcu("VerticalSpeed"), fcu("VerticalAngle"), fcu("VerticalManaged"), fcu("VerticalType"));
    }

    if (!loggedComposed && data.displayEnabled) {
        loggedComposed = true;
        Logger::getInstance()->info("FF A320 FCU windows: enabled=%d conduction=%f spd=%s hdg=%s alt=%s vs=%s baroL=%s\n",
            data.displayEnabled ? 1 : 0, conduction, data.speed.c_str(), data.heading.c_str(), data.altitude.c_str(),
            data.verticalSpeed.c_str(), data.efisLeft.baro.c_str());
    }

    snapshot = data;
}

void FFA320FCUEfisProfile::updateLeds() {
    static const struct {
            const char *element;
            FCUEfisLed led;
    } lamps[] = {
        {"FCU_LocalizerLight", FCUEfisLed::LOC_GREEN},
        {"FCU_AutoPilotLight1", FCUEfisLed::AP1_GREEN},
        {"FCU_AutoPilotLight2", FCUEfisLed::AP2_GREEN},
        {"FCU_AutoThrustLight", FCUEfisLed::ATHR_GREEN},
        {"FCU_ExpediteLight", FCUEfisLed::EXPED_GREEN},
        {"FCU_ApproachLight", FCUEfisLed::APPR_GREEN},

        {"EFIS_FlightDirLightL", FCUEfisLed::EFISL_FD_GREEN},
        {"EFIS_LandSysLightL", FCUEfisLed::EFISL_LS_GREEN},
        {"EFIS_NavTypeLight1L", FCUEfisLed::EFISL_CSTR_GREEN},
        {"EFIS_NavTypeLight2L", FCUEfisLed::EFISL_WPT_GREEN},
        {"EFIS_NavTypeLight3L", FCUEfisLed::EFISL_VORD_GREEN},
        {"EFIS_NavTypeLight4L", FCUEfisLed::EFISL_NDB_GREEN},
        {"EFIS_NavTypeLight5L", FCUEfisLed::EFISL_ARPT_GREEN},

        {"EFIS_FlightDirLightR", FCUEfisLed::EFISR_FD_GREEN},
        {"EFIS_LandSysLightR", FCUEfisLed::EFISR_LS_GREEN},
        {"EFIS_NavTypeLight1R", FCUEfisLed::EFISR_CSTR_GREEN},
        {"EFIS_NavTypeLight2R", FCUEfisLed::EFISR_WPT_GREEN},
        {"EFIS_NavTypeLight3R", FCUEfisLed::EFISR_VORD_GREEN},
        {"EFIS_NavTypeLight4R", FCUEfisLed::EFISR_NDB_GREEN},
        {"EFIS_NavTypeLight5R", FCUEfisLed::EFISR_ARPT_GREEN},
    };

    if (!loggedLampSignals) {
        loggedLampSignals = true;
        Logger::getInstance()->info("FF A320 FCU lamps: AP1 state=%f intensity=%f, digit state=%f intensity=%f\n",
            panel("FCU_AutoPilotLight1.State"), panel("FCU_AutoPilotLight1.Intensity"),
            panel("FCU_AltitudeDigit1.State"), panel("FCU_AltitudeDigit1.Intensity"));
    }

    // State is what the lamp is doing; Intensity is only how bright it would be
    // when lit, which is why every green came on at once when it drove them.
    for (const auto &lamp : lamps) {
        const std::string statePath = std::string(PanelPath) + lamp.element + ".State";
        const double lit = resolves(statePath) ? panel(std::string(lamp.element) + ".State") : panel(std::string(lamp.element) + ".Intensity");

        product->setLedBrightness(lamp.led, lit > 0.001 ? 1 : 0);
    }
}

void FFA320FCUEfisProfile::updateBrightness() {
    // The FCU and both EFIS units sit on the glareshield, so their bezels follow
    // its integral lighting rather than a rheostat of their own.
    const double integral = std::clamp(std::max(panel("IntegTarget1"), panel("IntegTarget2")), 0.0, 1.0);
    const uint8_t bezel = static_cast<uint8_t>(integral * 255);

    product->setLedBrightness(FCUEfisLed::BACKLIGHT, bezel);
    product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, bezel);
    product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, bezel);
    product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, bezel);

    const bool powered = snapshot.displayEnabled;

    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, powered ? 255 : 0);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, powered ? 255 : 0);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, powered ? 255 : 0);

    // The LCD glyphs carry their own lit intensity, which is what the pilot
    // actually dims, so the screen follows a digit rather than the bezel.
    // The glyph emissive only tracks night lighting, so the LCD follows the same
    // glareshield rheostat as the bezel and keeps a floor for daylight.
    constexpr uint8_t minimumScreenBrightness = 64;
    const uint8_t screen = powered ? static_cast<uint8_t>(minimumScreenBrightness + integral * (255 - minimumScreenBrightness)) : 0;

    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, screen);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, screen);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, screen);
}

const std::vector<std::string> &FFA320FCUEfisProfile::displayDatarefs() const {
    static std::unordered_map<std::string, std::vector<std::string>> cache;

    return cache.try_emplace(displayVersionRef, std::vector<std::string>{displayVersionRef}).first->second;
}

void FFA320FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    // Seeds the cache entry the display poll needs; without this read the
    // version dataref is never polled and the panel is written only once.
    Dataref::getInstance()->getCached<int>(displayVersionRef.c_str());

    data = snapshot;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &FFA320FCUEfisProfile::buttonDefs() const {
    // EXECUTE_CMD_ONCE carries an X-Plane command the aircraft publishes for a
    // click object, SET_VALUE a shared value leaf under Aircraft.Cockpit.Panel
    // written to an absolute detent, ADJUST_VALUE the STD/QNH reference and
    // BAROMETER_* one step of the baro knob.
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        {0, {"MACH", "a320/Panel/FCU_Mach_button"}},
        {1, {"LOC", "a320/Panel/FCU_Localizer_button"}},
        {2, {"TRK", "a320/Panel/FCU_Mode_button"}},
        {3, {"AP1", "a320/Panel/FCU_AutoPilot1_button"}},
        {4, {"AP2", "a320/Panel/FCU_AutoPilot2_button"}},
        {5, {"A/THR", "a320/Panel/FCU_AutoThrust_button"}},
        {6, {"EXPED", "a320/Panel/FCU_Expedite_button"}},
        {7, {"METRIC", "a320/Panel/FCU_Metric_button"}},
        {8, {"APPR", "a320/Panel/FCU_Approach_button"}},
        {9, {"SPD DEC", "a320/Panel/FCU_Speed_switch-"}},
        {10, {"SPD INC", "a320/Panel/FCU_Speed_switch+"}},
        {11, {"SPD PUSH", "a320/Panel/FCU_SpeedMode_switch_push"}},
        {12, {"SPD PULL", "a320/Panel/FCU_SpeedMode_switch_pull"}},
        {13, {"HDG DEC", "a320/Panel/FCU_Lateral_switch-"}},
        {14, {"HDG INC", "a320/Panel/FCU_Lateral_switch+"}},
        {15, {"HDG PUSH", "a320/Panel/FCU_LateralMode_switch_push"}},
        {16, {"HDG PULL", "a320/Panel/FCU_LateralMode_switch_pull"}},
        {17, {"ALT DEC", "a320/Panel/FCU_Altitude_switch-"}},
        {18, {"ALT INC", "a320/Panel/FCU_Altitude_switch+"}},
        {19, {"ALT PUSH", "a320/Panel/FCU_AltitudeMode_switch_push"}},
        {20, {"ALT PULL", "a320/Panel/FCU_AltitudeMode_switch_pull"}},
        {21, {"VS DEC", "a320/Panel/FCU_Vertical_switch-"}},
        {22, {"VS INC", "a320/Panel/FCU_Vertical_switch+"}},
        {23, {"VS PUSH", "a320/Panel/FCU_VerticalMode_switch_push"}},
        {24, {"VS PULL", "a320/Panel/FCU_VerticalMode_switch_pull"}},
        {25, {"ALT 100", "altstep", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {26, {"ALT 1000", "altstep", FCUEfisDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},

        // Buttons 27-31 reserved

        {32, {"L_FD", "a320/Panel/EFIS_FlightDirL_button"}},
        {33, {"L_LS", "a320/Panel/EFIS_LandSysL_button"}},
        {34, {"L_CSTR", "a320/Panel/EFIS_NavType1L_button"}},
        {35, {"L_WPT", "a320/Panel/EFIS_NavType2L_button"}},
        {36, {"L_VOR.D", "a320/Panel/EFIS_NavType3L_button"}},
        {37, {"L_NDB", "a320/Panel/EFIS_NavType4L_button"}},
        {38, {"L_ARPT", "a320/Panel/EFIS_NavType5L_button"}},
        {39, {"L_STD PUSH", "L", FCUEfisDatarefType::ADJUST_VALUE, 0.0}},
        {40, {"L_STD PULL", "L", FCUEfisDatarefType::ADJUST_VALUE, 1.0}},
        {41, {"L_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, -1.0}},
        {42, {"L_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_PILOT, 1.0}},
        {43, {"L_inHg", "EFIS_BaroTypeL.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {44, {"L_hPa", "EFIS_BaroTypeL.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {45, {"L_MODE LS", "EFIS_NavModeL.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {46, {"L_MODE VOR", "EFIS_NavModeL.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {47, {"L_MODE NAV", "EFIS_NavModeL.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {48, {"L_MODE ARC", "EFIS_NavModeL.Target", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {49, {"L_MODE PLAN", "EFIS_NavModeL.Target", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {50, {"L_RANGE 10", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {51, {"L_RANGE 20", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {52, {"L_RANGE 40", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {53, {"L_RANGE 80", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {54, {"L_RANGE 160", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {55, {"L_RANGE 320", "EFIS_NavRangeL.Target", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {56, {"L_1 ADF", "EFIS_NavReciver1L.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {57, {"L_1 OFF", "EFIS_NavReciver1L.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {58, {"L_1 VOR", "EFIS_NavReciver1L.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {59, {"L_2 ADF", "EFIS_NavReciver2L.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {60, {"L_2 OFF", "EFIS_NavReciver2L.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {61, {"L_2 VOR", "EFIS_NavReciver2L.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},

        // Buttons 62-63 reserved

        {64, {"R_FD", "a320/Panel/EFIS_FlightDirR_button"}},
        {65, {"R_LS", "a320/Panel/EFIS_LandSysR_button"}},
        {66, {"R_CSTR", "a320/Panel/EFIS_NavType1R_button"}},
        {67, {"R_WPT", "a320/Panel/EFIS_NavType2R_button"}},
        {68, {"R_VOR.D", "a320/Panel/EFIS_NavType3R_button"}},
        {69, {"R_NDB", "a320/Panel/EFIS_NavType4R_button"}},
        {70, {"R_ARPT", "a320/Panel/EFIS_NavType5R_button"}},
        {71, {"R_STD PUSH", "R", FCUEfisDatarefType::ADJUST_VALUE, 0.0}},
        {72, {"R_STD PULL", "R", FCUEfisDatarefType::ADJUST_VALUE, 1.0}},
        {73, {"R_PRESS DEC", "custom", FCUEfisDatarefType::BAROMETER_FO, -1.0}},
        {74, {"R_PRESS INC", "custom", FCUEfisDatarefType::BAROMETER_FO, 1.0}},
        {75, {"R_inHg", "EFIS_BaroTypeR.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {76, {"R_hPa", "EFIS_BaroTypeR.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {77, {"R_MODE LS", "EFIS_NavModeR.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {78, {"R_MODE VOR", "EFIS_NavModeR.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {79, {"R_MODE NAV", "EFIS_NavModeR.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {80, {"R_MODE ARC", "EFIS_NavModeR.Target", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {81, {"R_MODE PLAN", "EFIS_NavModeR.Target", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {82, {"R_RANGE 10", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {83, {"R_RANGE 20", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {84, {"R_RANGE 40", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {85, {"R_RANGE 80", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 3.0}},
        {86, {"R_RANGE 160", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 4.0}},
        {87, {"R_RANGE 320", "EFIS_NavRangeR.Target", FCUEfisDatarefType::SET_VALUE, 5.0}},
        {88, {"R_1 VOR", "EFIS_NavReciver1R.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {89, {"R_1 OFF", "EFIS_NavReciver1R.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {90, {"R_1 ADF", "EFIS_NavReciver1R.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        {91, {"R_2 VOR", "EFIS_NavReciver2R.Target", FCUEfisDatarefType::SET_VALUE, 2.0}},
        {92, {"R_2 OFF", "EFIS_NavReciver2R.Target", FCUEfisDatarefType::SET_VALUE, 1.0}},
        {93, {"R_2 ADF", "EFIS_NavReciver2R.Target", FCUEfisDatarefType::SET_VALUE, 0.0}},
        // Buttons 94-95 reserved
    };

    return buttons;
}

// MACH, TRK and METRIC answer their published command; LOC, AP1, AP2, A/THR,
// EXPED and APPR do not, and those six are the ones the FMGC reads rather than
// the FCU itself. Their click object is pressed the way the mouse presses it,
// and the two that carry a pressed leaf of their own get that as well.
void FFA320FCUEfisProfile::pressClickObject(const std::string &command, bool down) {
    static const std::unordered_map<std::string, std::pair<const char *, const char *>> fallbacks = {
        {"a320/Panel/FCU_Localizer_button", {"FCU_Localizer", nullptr}},
        {"a320/Panel/FCU_Approach_button", {"FCU_Approach", nullptr}},
        {"a320/Panel/FCU_Expedite_button", {"FCU_Expedite", nullptr}},
        {"a320/Panel/FCU_AutoPilot1_button", {"FCU_AutoPilot1", "AutoPilotPressed"}},
        {"a320/Panel/FCU_AutoPilot2_button", {"FCU_AutoPilot2", "AutoPilotPressed"}},
        {"a320/Panel/FCU_AutoThrust_button", {"FCU_AutoThrust", "AutoThrustPressed"}},
    };

    auto fallback = fallbacks.find(command);
    if (fallback == fallbacks.end()) {
        return;
    }

    auto shared = FFSharedValues::getInstance();

    // Target is the leaf the EFIS selectors already answer on this aircraft, so
    // the pushbuttons get both it and Click.
    shared->setNumber(std::string(PanelPath) + fallback->second.first + ".Target", down ? 1.0 : 0.0);
    shared->setNumber(std::string(PanelPath) + fallback->second.first + ".Click", down ? 1.0 : 0.0);

    if (fallback->second.second) {
        // One leaf covers both autopilots, so it carries which one was pressed.
        const double pressed = command == "a320/Panel/FCU_AutoPilot2_button" ? 2.0 : 1.0;
        shared->setNumber(fcuPath + fallback->second.second, down ? pressed : 0.0);
    }
}

// The switch is maintained, so the hardware reports its detent only when it
// changes. A detent already held at startup therefore never reaches the
// aircraft, which is why beginning a session on 100 stepped in thousands. The
// press latches the wanted detent and the poll asserts it once the tree is up.
void FFA320FCUEfisProfile::setAltitudeStep(bool thousands) {
    wantedAltitudeStep = thousands ? 1 : 0;
    syncAltitudeStep();
}

// Only a "switch+" is published, so the detent is reached by toggling when the
// made contact is the wrong one. State1 is the 100 position, State2 the 1000.
void FFA320FCUEfisProfile::syncAltitudeStep() {
    if (wantedAltitudeStep < 0) {
        return;
    }

    const std::string object = std::string(PanelPath) + "FCU_AltitudeStep.";
    if (!resolves(object + "State2")) {
        return;
    }

    const bool onThousands = FFSharedValues::getInstance()->getNumber(object + "State2") > 0.5;
    if (onThousands == (wantedAltitudeStep == 1)) {
        return;
    }

    // The toggle takes a moment to register, so leave it time before retrying.
    const float now = XPLMGetElapsedTime();
    if (now - lastAltitudeStepToggle < 0.5f) {
        return;
    }

    lastAltitudeStepToggle = now;
    runCommand("a320/Panel/FCU_AltitudeStep_switch+");
}

// The mode knobs take Target +1 for pull and -1 for push around a rest of 0.
// FFA320Connector uses the opposite sign, but on the Ultimate that made pull act
// as push. Rest is fixed, since the published command may already have moved
// the knob. The aircraft samples it per frame, so the pulse is held past release.
void FFA320FCUEfisProfile::pressModeSwitch(const std::string &command, bool down) {
    const size_t start = command.find("FCU_");
    const size_t end = command.find("_switch_");
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return;
    }

    const std::string target = std::string(PanelPath) + command.substr(start, end - start) + ".Target";
    auto shared = FFSharedValues::getInstance();

    if (!down) {
        auto pulse = modeSwitchPulses.find(target);
        if (pulse != modeSwitchPulses.end()) {
            pulse->second.released = true;
        }
        return;
    }

    ModeSwitchPulse pulse;
    pulse.pressedAt = XPLMGetElapsedTime();
    modeSwitchPulses[target] = pulse;

    shared->setNumber(target, command.ends_with("_pull") ? 1.0 : -1.0);
}

void FFA320FCUEfisProfile::releaseModeSwitches() {
    const float now = XPLMGetElapsedTime();
    auto shared = FFSharedValues::getInstance();

    for (auto it = modeSwitchPulses.begin(); it != modeSwitchPulses.end();) {
        if (it->second.released && now - it->second.pressedAt >= 0.1f) {
            shared->setNumber(it->first, 0.0);
            it = modeSwitchPulses.erase(it);
        } else {
            ++it;
        }
    }
}

bool FFA320FCUEfisProfile::resolves(const std::string &path) const {
    return FFSharedValues::getInstance()->idForPath(path) >= 0;
}

bool FFA320FCUEfisProfile::runCommand(const std::string &command) {
    XPLMCommandRef handle = XPLMFindCommand(command.c_str());
    if (!handle) {
        if (loggedMissingCommands.insert(command).second) {
            Logger::getInstance()->warn("FF A320: command \"%s\" does not exist.\n", command.c_str());
        }

        return false;
    }

    XPLMCommandOnce(handle);

    return true;
}

void FFA320FCUEfisProfile::stepBaro(bool captainSide, bool increase) {
    const std::string side = captainSide ? "L" : "R";

    // The knob's own command keeps the aircraft's step and unit handling; the
    // published command list only documents the captain side, so the first
    // officer's falls back to the value the knob would have written.
    if (runCommand("a320/Panel/EFIS_Baro" + side + "_switch" + (increase ? "+" : "-"))) {
        return;
    }

    const std::string path = fcuPath + "Baro" + side;
    const double baro = FFSharedValues::getInstance()->getNumber(path);
    if (baro <= 0.0) {
        return;
    }

    // The leaf counts inHg in hundredths and hPa whole, so one step is one
    // count in either unit.
    FFSharedValues::getInstance()->setNumber(path, baro + (increase ? 1.0 : -1.0));
}

void FFA320FCUEfisProfile::setBaroStd(bool captainSide, bool standard) {
    const std::string side = captainSide ? "L" : "R";

    if (runCommand("a320/Panel/EFIS_BaroMode" + side + "_switch_" + (standard ? "pull" : "push"))) {
        return;
    }

    FFSharedValues::getInstance()->setNumber(fcuPath + "BaroMode" + side, standard ? 1.0 : 0.0);
}

// A click object the aircraft animates down and back up. Firing it as one
// instant command left the cockpit button latched and unclickable, so press and
// release are carried through as the real button's are.
static bool isMomentaryClick(const std::string &command) {
    return command.ends_with("_button") || command.ends_with("_switch_push") || command.ends_with("_switch_pull");
}

void FFA320FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == FCUEfisDatarefType::EXECUTE_CMD_ONCE && isMomentaryClick(button->dataref)) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
        pressClickObject(button->dataref, phase == xplm_CommandBegin);

        if (button->dataref.ends_with("_switch_pull") || button->dataref.ends_with("_switch_push")) {
            pressModeSwitch(button->dataref, phase == xplm_CommandBegin);
        }
        return;
    }

    if (phase != xplm_CommandBegin) {
        return;
    }

    switch (button->datarefType) {
        case FCUEfisDatarefType::SET_VALUE_USING_COMMANDS:
            setAltitudeStep(button->value > 0.5);
            break;

        case FCUEfisDatarefType::SET_VALUE:
            FFSharedValues::getInstance()->setNumber(PanelPath + button->dataref, button->value);
            break;

        case FCUEfisDatarefType::ADJUST_VALUE:
            setBaroStd(button->dataref == "L", button->value > 0.5);
            break;

        case FCUEfisDatarefType::BAROMETER_PILOT:
            stepBaro(true, button->value > 0);
            break;

        case FCUEfisDatarefType::BAROMETER_FO:
            stepBaro(false, button->value > 0);
            break;

        default:
            runCommand(button->dataref);
            break;
    }
}

#include "atr-fcu-efis-profile.h"

#include "dataref.h"
#include "product-fcu-efis.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <XPLMUtilities.h>

// ---------------------------------------------------------------------------
// Dataref dell'ATR 72-600 (DeltaWing). Da verificare in DataRefTool.
// ---------------------------------------------------------------------------
static const std::string ATR_ALT_SEL = "DWSim/ATR_Series/afcs/alt_sel_ft";
static const std::string ATR_HDG_SEL = "DWSim/ATR_Series/afcs/hdg_sel_deg";



// DA COMPLETARE: nomi dei dataref di velocita' (IAS) e V/S selezionate.
// Finche' restano vuoti, il display relativo mostra solo zeri.
static const std::string ATR_SPD_SEL = "DWSim/ATR_Series/afcs/spd_target_kt";
static const std::string ATR_VS_SEL = "DWSim/ATR_Series/afcs/vs_target_fpm";

// Baro: dataref standard di X-Plane, come nel profilo Q4XP.
// Se l'ATR usa dataref propri, sostituirli qui.
static const std::string ATR_BARO_LEFT = "sim/cockpit/misc/barometer_setting";
static const std::string ATR_BARO_RIGHT = "sim/cockpit/misc/barometer_setting2";

// Tipo di ciascun dataref (float o int, lo mostra DataRefTool).
// Se un dataref e' int, cambiare qui il tipo corrispondente in int.
using AltSelType = float;
using HdgSelType = float;
using SpdSelType = float;
using VsSelType = float;
using BaroType = float;

// Luminosita' fissa dei LED di retroilluminazione (0-255).
// Il Q4XP la legge da un dataref luci del cockpit; per l'ATR e' fissa.
static constexpr uint8_t BUTTON_BACKLIGHT = 255;
static constexpr uint8_t SCREEN_BACKLIGHT = 255;

// Legge un dataref opzionale: fallback se il nome e' vuoto o il dataref non esiste.
template <typename T>
static float readOptional(const std::string &name, float fallback = 0.0f) {
    if (name.empty()) {
        return fallback;
    }

    auto dm = Dataref::getInstance();
    if (!dm->exists(name.c_str())) {
        return fallback;
    }

    return static_cast<float>(dm->getCached<T>(name.c_str()));
}

ATRFCUEfisProfile::ATRFCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    XPLMDebugString("WINCTRL: ATR FCU/EFIS profile loaded\n");

    product->setLedBrightness(FCUEfisLed::BACKLIGHT, BUTTON_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, BUTTON_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, BUTTON_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, BUTTON_BACKLIGHT);

    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, SCREEN_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, SCREEN_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, SCREEN_BACKLIGHT);
    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 255);
    product->forceStateSync();
}

ATRFCUEfisProfile::~ATRFCUEfisProfile() {
}

bool ATRFCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("DWSim/ATR_Series/afcs/alt_sel_ft");
}

const std::vector<std::string> &ATRFCUEfisProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = [] {
        std::vector<std::string> list = {
            ATR_ALT_SEL,
            ATR_HDG_SEL,
            ATR_BARO_LEFT,
            ATR_BARO_RIGHT,
        };
        if (!ATR_SPD_SEL.empty()) {
            list.push_back(ATR_SPD_SEL);
        }
        if (!ATR_VS_SEL.empty()) {
            list.push_back(ATR_VS_SEL);
        }
        return list;
    }();
    return datarefs;
}

const std::unordered_map<uint16_t, FCUEfisButtonDef> &ATRFCUEfisProfile::buttonDefs() const {
    // Pulsanti e manopole sono gestiti altrove: nessuna associazione qui.
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU buttons
        {3,  {"AP1",  "DWSim/ATR_Series/commands/front/afcs/apPushButton"}},
        {4,  {"AP2",  ""}},
        {7,  {"METRIC", ""}},

        // Speed encoder
        {9,  {"SPD DEC",  "DWSim/ATR_Series/commands/front/icp/left/spdTgtRotationKnob/rotateLeft"}},
        {10, {"SPD INC",  "DWSim/ATR_Series/commands/front/icp/left/spdTgtRotationKnob/rotateRight"}},
        {11, {"SPD PUSH", "DWSim/ATR_Series/commands/front/afcs/iasPushButton"}},
        {12, {"SPD PULL", "DWSim/ATR_Series/commands/front/icp/left/autoManPushButton/toggle"}},

        // Heading encoder
        {13, {"HDG DEC",  "DWSim/ATR_Series/commands/front/afcs/headingKnob/rotateLeft"}},
        {14, {"HDG INC",  "DWSim/ATR_Series/commands/front/afcs/headingKnob/rotateRight"}},
        {15, {"HDG PUSH", "DWSim/ATR_Series/commands/front/afcs/hdgPushButton"}},
        {16, {"HDG PULL", ""}},

        // Altitude encoder
        {17, {"ALT DEC",  "DWSim/ATR_Series/commands/front/afcs/altSel/rotateLeft"}},
        {18, {"ALT INC",  "DWSim/ATR_Series/commands/front/afcs/altSel/rotateRight"}},
        {19, {"ALT PUSH", "DWSim/ATR_Series/commands/front/afcs/vnavPushButton"}},
        {20, {"ALT PULL", "DWSim/ATR_Series/commands/front/afcs/altPushButton"}},

        // Vertical Speed encoder
        {21, {"VS DEC",  "DWSim/ATR_Series/commands/front/afcs/vsSpeed/rotateDn"}},
        {22, {"VS INC",  "DWSim/ATR_Series/commands/front/afcs/vsSpeed/rotateUp"}},
        {23, {"VS PUSH", "DWSim/ATR_Series/commands/front/afcs/vsPushButton"}},
        {24, {"VS PULL", ""}},

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

void ATRFCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    data.displayEnabled = true;

    // Speed
    float spd = readOptional<SpdSelType>(ATR_SPD_SEL);
    {
        int spdInt = std::max(0, static_cast<int>(std::round(spd)));
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << spdInt;
        data.speed = oss.str();
    }
    data.spdMach = false;
    data.spdManaged = false;

    // Heading
    float hdg = readOptional<HdgSelType>(ATR_HDG_SEL);
    {
        int hdgInt = static_cast<int>(std::round(hdg)) % 360;
        if (hdgInt < 0) {
            hdgInt += 360;
        }
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << hdgInt;
        data.heading = oss.str();
    }
    data.headingHdg = true;
    data.headingTrk = false;
    data.headingLat = false;
    data.hdgManaged = false;

    // Altitude
    float alt = readOptional<AltSelType>(ATR_ALT_SEL);
    {
        int altInt = std::clamp(static_cast<int>(std::round(alt)), 0, 99999);
        std::ostringstream oss;
        oss << std::internal << std::setw(5) << std::setfill('0') << altInt;
        data.altitude = oss.str();
    }
    data.altManaged = false;
    data.altIndication = true;
    data.lvlChange = false;

    // Vertical Speed
    float vs = readOptional<VsSelType>(ATR_VS_SEL);
    {
        bool negative = vs < 0;
        int vsAbs = std::min(9999, static_cast<int>(std::round(std::abs(vs))));
        std::ostringstream oss;
        oss << std::setw(4) << std::setfill('0') << vsAbs;
        data.verticalSpeed = oss.str();
        data.vsSign = negative;
    }
    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    data.vsHorizontalLine = false;
    data.vsVerticalLine = false;

    // EFIS Baro (stessa gestione del profilo Q4XP; 29.92 se il dataref manca)
    float baro1 = readOptional<BaroType>(ATR_BARO_LEFT, 29.92f);
    data.efisLeft.displayEnabled = true;
    data.efisLeft.unitIsInHg = false;
    data.efisLeft.setBaro(baro1, false);

    float baro2 = readOptional<BaroType>(ATR_BARO_RIGHT, 29.92f);
    data.efisRight.displayEnabled = true;
    data.efisRight.unitIsInHg = false;
    data.efisRight.setBaro(baro2, false);
}

void ATRFCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

#include "dwsim-atr72-fcu-efis-profile.h"

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
static const std::string ATR_CRS_CAPT_SEL = "DWSim/ATR_Series/afcs/crs_capt_deg";
static const std::string ATR_SPD_MODE = "DWSim/ATR_Series/afcs/spd_mode_auto";

//static const std::string ATR_CRS_FO_SEL = "DWSim/ATR_Series/afcs/crs_fo_deg";

static const std::string ATR_SPD_SEL = "DWSim/ATR_Series/afcs/spd_target_kt";
static const std::string ATR_VS_SEL = "DWSim/ATR_Series/afcs/vs_target_fpm";
static const std::string ATR_BARO_LEFT = "sim/cockpit/misc/barometer_setting";
static const std::string ATR_BARO_RIGHT = "sim/cockpit/misc/barometer_setting2";

//DWSim/ATR_Series/datarefs/front/afcs/vilsKnobLeft/rotation (0=FMC1,1=ILS1,2=ILS2, 3=FMC2)
//DWSim/ATR_Series/afcs/crs_capt_deg

// Tipo di ciascun dataref (float o int, lo mostra DataRefTool).
// Se un dataref e' int, cambiare qui il tipo corrispondente in int.
// using AltSelType = float;
// using HdgSelType = float;
// using SpdSelType = float;
// using VsSelType = float;
// using BaroType = float;

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

// Converte una luminosita' da 0..1 (anche 0.2, 0.3, ...) in un valore LED 0..255.
// Valori fuori scala vengono limitati; valori non numerici diventano 0.
static uint8_t brightnessToLed(float level) {
    if (!std::isfinite(level)) {
        return 0;
    }

    float clamped = std::clamp(level, 0.0f, 1.0f);
    long scaled = std::lround(clamped * 255.0f);
    return static_cast<uint8_t>(std::clamp(scaled, 0L, 255L));
}

DWSimATR72FCUEfisProfile::DWSimATR72FCUEfisProfile(ProductFCUEfis *product) : FCUEfisAircraftProfile(product) {
    XPLMDebugString("WINCTRL: ATR FCU/EFIS profile loaded\n");


// Retroilluminazione pulsanti: il dataref fornisce un valore da 0 a 1,
    Dataref::getInstance()->monitorExistingDataref<float>("DWSim/ATR_Series/lit/cockpit/mipPed/brightness", [product](float brightness) {
        float level = std::isfinite(brightness) ? std::clamp(brightness, 0.0f, 1.0f) : 0.0f;
        uint8_t target = static_cast<uint8_t>(std::clamp(std::lround(level * 255.0f), 0L, 255L));
        product->setLedBrightness(FCUEfisLed::BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EXPED_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISL_BACKLIGHT, target);
        product->setLedBrightness(FCUEfisLed::EFISR_BACKLIGHT, target);
        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>("DWSim/ATR_Series/afcs/ap_engaged", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::AP1_GREEN, engaged ? 255 : 0);
    },
        this);
        
    Dataref::getInstance()->monitorExistingDataref<int>("DWSim/ATR_Series/afcs/yd_engaged", [product](int engaged) {
        product->setLedBrightness(FCUEfisLed::ATHR_GREEN, engaged ? 255 : 0);
    },
        this);
    
    Dataref::getInstance()->monitorExistingDataref<std::string>("DWSim/ATR_Series/afcs/fma_lat_active", [product](std::string engaged) {
        product->setLedBrightness(FCUEfisLed::LOC_GREEN, engaged == "LNAV" ? 255 : 0
        );
    },
        this);

    //
        
    product->setLedBrightness(FCUEfisLed::SCREEN_BACKLIGHT, 255);
    product->setLedBrightness(FCUEfisLed::EFISL_SCREEN_BACKLIGHT, 255);
    product->setLedBrightness(FCUEfisLed::EFISR_SCREEN_BACKLIGHT, 255);
    product->setLedBrightness(FCUEfisLed::OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISL_OVERALL_GREEN, 255);
    product->setLedBrightness(FCUEfisLed::EFISR_OVERALL_GREEN, 255);

    product->forceStateSync();
}

DWSimATR72FCUEfisProfile::~DWSimATR72FCUEfisProfile() {
}

bool DWSimATR72FCUEfisProfile::IsEligible() {
    return Dataref::getInstance()->exists("DWSim/ATR_Series/afcs/alt_sel_ft");
}

const std::vector<std::string> &DWSimATR72FCUEfisProfile::displayDatarefs() const {
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

const std::unordered_map<uint16_t, FCUEfisButtonDef> &DWSimATR72FCUEfisProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, FCUEfisButtonDef> buttons = {
        // FCU buttons
        {0, {"MACH", "DWSim/ATR_Series/commands/front/133vm/warnPushButton/toggle"}},
        {1, {"LOC", "DWSim/ATR_Series/commands/front/afcs/navPushButton"}},
        {3, {"AP1",  "DWSim/ATR_Series/commands/front/afcs/apPushButton"}},
        // {4, {"AP2",  "DWSim/ATR_Series/commands/front/afcs/apPushButton"}},
        {5, {"A/THR", "DWSim/ATR_Series/commands/front/afcs/ydPushButton"}},
        {6, {"EXPED", "DWSim/ATR_Series/commands/front/afcs/bcPushButton"}},
        {7, {"METRIC", ""}},
        {8, {"APPR", "DWSim/ATR_Series/commands/front/afcs/appPushButton"}},

        // Speed encoder
        {9, {"SPD DEC",  "DWSim/ATR_Series/commands/front/icp/left/spdTgtRotationKnob/rotateLeft"}},
        {10, {"SPD INC",  "DWSim/ATR_Series/commands/front/icp/left/spdTgtRotationKnob/rotateRight"}},
        {11, {"SPD PUSH", "DWSim/ATR_Series/commands/front/icp/left/autoManPushButton/toggle"}},
        {12, {"SPD PULL", "DWSim/ATR_Series/commands/front/icp/left/autoManPushButton/toggle"}},

        // Heading encoder
        {13, {"HDG DEC",  "DWSim/ATR_Series/commands/front/afcs/headingKnob/rotateLeft"}},
        {14, {"HDG INC",  "DWSim/ATR_Series/commands/front/afcs/headingKnob/rotateRight"}},
        {15, {"HDG PUSH", "DWSim/ATR_Series/commands/front/afcs/headingKnob/push"}},
        {16, {"HDG PULL", "DWSim/ATR_Series/commands/front/afcs/hdgPushButton"}},

        // Altitude encoder
        {17, {"ALT DEC",  "DWSim/ATR_Series/commands/front/afcs/altSel/rotateLeft"}},
        {18, {"ALT INC",  "DWSim/ATR_Series/commands/front/afcs/altSel/rotateRight"}},
        {19, {"ALT PUSH", "DWSim/ATR_Series/commands/front/afcs/vnavPushButton"}},
        {20, {"ALT PULL", "DWSim/ATR_Series/commands/front/afcs/iasPushButton"}},

        // Vertical Speed encoder
        {21, {"VS DEC",  "DWSim/ATR_Series/commands/front/afcs/vsSpeed/rotateDn"}},
        {22, {"VS INC",  "DWSim/ATR_Series/commands/front/afcs/vsSpeed/rotateUp"}},
        {23, {"VS PUSH", "DWSim/ATR_Series/commands/front/afcs/altPushButton"}},
        {24, {"VS PULL", "DWSim/ATR_Series/commands/front/afcs/vsPushButton"}},

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
/*

*/
void DWSimATR72FCUEfisProfile::updateDisplayData(FCUDisplayData &data) {
    data.displayEnabled = true;

    //DWSim/ATR_Series/afcs/spd_target_kt
    // Speed
    float spd = readOptional<float>(ATR_SPD_SEL);
    {
        int spdInt = std::max(0, static_cast<int>(std::round(spd)));
        std::ostringstream oss;
        oss << std::setw(3) << std::setfill('0') << spdInt;
        data.speed = oss.str();
    }

    //int spdMng = readOptional<int>(ATR_SPD_MODE);
    //DWSim/ATR_Series/afcs/spd_mode_auto 1=managed, 0=unmanaged
    
    data.spdManaged = readOptional<int>(ATR_SPD_MODE) == 1 ? true : false;
    data.spdMach = false;

    // Heading
    float hdg = readOptional<float>(ATR_HDG_SEL);
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
    float alt = readOptional<float>(ATR_ALT_SEL);
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
    float vs = readOptional<float>(ATR_VS_SEL);
    {
        bool negative = vs <= 0;
        int vsAbs = std::min(9999, static_cast<int>(std::round(std::abs(vs))));
        std::ostringstream oss;
        
        // oss << std::setw(4) << std::setfill('0') << vsAbs;
        // If VS is a multiple of 100, show last two digits as "##"
        
        if (vsAbs % 100 == 0) {
            oss << std::setfill('0') << std::setw(2) << (vsAbs / 100) << "##";
        } else {
            // Show full value for non-multiples of 100
            oss << std::setfill('0') << std::setw(4) << vsAbs;
        }
        data.verticalSpeed = oss.str();
        data.vsSign = !negative;
        data.vsHorizontalLine = vs == 0 ? false : true; // Show horizontal line for negative VS, hide when zero
    }
    data.vsMode = true;
    data.fpaMode = false;
    data.vsIndication = true;
    // data.vsHorizontalLine = true;
    data.vsVerticalLine = false;

    // EFIS Baro
    float baro1 = readOptional<float>(ATR_BARO_LEFT, 29.92f);
    data.efisLeft.displayEnabled = true;
    data.efisLeft.unitIsInHg = false;
    data.efisLeft.setBaro(baro1, false);

    float baro2 = readOptional<float>(ATR_BARO_RIGHT, 29.92f);
    data.efisRight.displayEnabled = true;
    data.efisRight.unitIsInHg = false;
    data.efisRight.setBaro(baro2, false);
}

void DWSimATR72FCUEfisProfile::buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (phase == xplm_CommandBegin) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

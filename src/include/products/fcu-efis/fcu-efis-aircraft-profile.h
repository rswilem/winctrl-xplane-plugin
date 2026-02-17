#ifndef FCUEFIS_AIRCRAFT_PROFILE_H
#define FCUEFIS_AIRCRAFT_PROFILE_H

#include "segment-display.h"

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <XPLMUtilities.h>

enum class DisplayByteIndex : int {
    H0 = 0,
    H3 = 1,
    A0 = 2,
    A1 = 3,
    A2 = 4,
    A3 = 5,
    A4 = 6,
    A5 = 7,
    V2 = 8,
    V3 = 9,
    V0 = 10,
    V1 = 11,
    S1 = 12,
    EFISR_B0 = 13,
    EFISR_B2 = 14,
    EFISL_B0 = 15,
    EFISL_B2 = 16
};

struct DisplayFlag {
        std::string name;
        DisplayByteIndex byteIndex;
        uint8_t mask;
        bool defaultValue;

        DisplayFlag(const std::string &n, DisplayByteIndex idx, uint8_t m, bool def = false) :
            name(n), byteIndex(idx), mask(m), defaultValue(def) {}
};

enum class FCUEfisDatarefType : unsigned char {
    SET_VALUE = 1,
    SET_VALUE_USING_COMMANDS,
    TOGGLE_VALUE,
    EXECUTE_CMD_ONCE,
    BAROMETER_PILOT,
    BAROMETER_FO,
    PUSH_BUTTON,
};

struct FCUEfisButtonDef {
        std::string name;
        std::string dataref;
        FCUEfisDatarefType datarefType = FCUEfisDatarefType::EXECUTE_CMD_ONCE;
        double value = 0.0;
};

enum class FCUEfisLed : int {
    // FCU LEDs
    BACKLIGHT = 0,
    SCREEN_BACKLIGHT = 1,
    OVERALL_GREEN = 2,

    _FCU_START = 3,
    LOC_GREEN = 3,
    AP1_GREEN = 5,
    AP2_GREEN = 7,
    ATHR_GREEN = 9,
    EXPED_GREEN = 11,
    APPR_GREEN = 13,
    EXPED_BACKLIGHT = 30,
    _FCU_END = 30,

    // EFIS Right LEDs (100-199)
    EFISR_BACKLIGHT = 100,
    EFISR_SCREEN_BACKLIGHT = 101,
    EFISR_OVERALL_GREEN = 102,

    _EFISR_START = 103,
    EFISR_FD_GREEN = 103,
    EFISR_LS_GREEN = 104,
    EFISR_CSTR_GREEN = 105,
    EFISR_WPT_GREEN = 106,
    EFISR_VORD_GREEN = 107,
    EFISR_NDB_GREEN = 108,
    EFISR_ARPT_GREEN = 109,
    _EFISR_END = 109,

    // EFIS Left LEDs (200-299)
    EFISL_BACKLIGHT = 200,
    EFISL_SCREEN_BACKLIGHT = 201,
    EFISL_OVERALL_GREEN = 202,

    _EFISL_START = 203,
    EFISL_FD_GREEN = 203,
    EFISL_LS_GREEN = 204,
    EFISL_CSTR_GREEN = 205,
    EFISL_WPT_GREEN = 206,
    EFISL_VORD_GREEN = 207,
    EFISL_NDB_GREEN = 208,
    EFISL_ARPT_GREEN = 209,
    _EFISL_END = 209
};

struct EfisDisplayValue {
        bool displayEnabled = true;
        bool displayTest = false;
        std::string baro;
        bool unitIsInHg;
        bool isStd = false;
        bool showQfe = false;

        bool operator==(const EfisDisplayValue &other) const {
            return showQfe == other.showQfe && baro == other.baro && unitIsInHg == other.unitIsInHg && isStd == other.isStd && displayEnabled == other.displayEnabled && displayTest == other.displayTest;
        }

        void setBaro(float inHgValue, bool isBaroInHg) {
            // Either make QNH hPa value (1013), or inHg * 100 (2992)
            isStd = false;
            int baroValue = static_cast<int>(std::round(inHgValue * (isBaroInHg ? 100.0f : 33.8639f)));
            unitIsInHg = isBaroInHg;
            std::ostringstream oss;
            oss << std::setw(4) << std::setfill(' ') << baroValue;
            baro = oss.str();
        }
};

struct FCUDisplayData {
        std::string speed;
        std::string heading;
        std::string altitude;
        std::string verticalSpeed;
        EfisDisplayValue efisLeft;
        EfisDisplayValue efisRight;

        bool displayEnabled = true;
        bool displayTest = false;

        // Display flags
        bool spdMach = false;
        bool hdgTrk = false;
        bool altManaged = false;
        bool spdManaged = false;
        bool hdgManaged = false;
        bool vsMode = false;
        bool fpaMode = false;

        // Additional display flags for proper 7-segment display
        bool latMode = false;
        bool altIndication = true;
        bool vsHorizontalLine = true;
        bool vsVerticalLine = false;
        bool lvlChange = true;
        bool lvlChangeLeft = true;
        bool lvlChangeRight = true;
        bool vsIndication = false;
        bool fpaIndication = false;
        bool fpaComma = false;
        bool vsSign = true; // true = positive (up), false = negative (down)

        bool operator==(const FCUDisplayData &other) const {
            return speed == other.speed &&
                   heading == other.heading &&
                   altitude == other.altitude &&
                   verticalSpeed == other.verticalSpeed &&
                   spdMach == other.spdMach &&
                   hdgTrk == other.hdgTrk &&
                   altManaged == other.altManaged &&
                   spdManaged == other.spdManaged &&
                   hdgManaged == other.hdgManaged &&
                   vsMode == other.vsMode &&
                   fpaMode == other.fpaMode &&
                   displayEnabled == other.displayEnabled &&
                   displayTest == other.displayTest;
        }
};

class ProductFCUEfis;

class FCUEfisAircraftProfile {
    protected:
        ProductFCUEfis *product;

    public:
        FCUEfisAircraftProfile(ProductFCUEfis *product) :
            product(product) {};
        virtual ~FCUEfisAircraftProfile() = default;

        virtual const std::vector<std::string> &displayDatarefs() const = 0;
        virtual const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const = 0;
        virtual void updateDisplayData(FCUDisplayData &displayData) = 0;
        virtual void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) = 0;
        virtual bool hasEfisRight() const = 0;
        virtual bool hasEfisLeft() const = 0;
};

#endif

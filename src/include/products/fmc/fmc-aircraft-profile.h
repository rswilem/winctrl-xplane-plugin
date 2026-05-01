#ifndef FMC_AIRCRAFT_PROFILE_H
#define FMC_AIRCRAFT_PROFILE_H

#include "fmc-hardware-mapping.h"

#include <array>
#include <map>
#include <string>
#include <unordered_map>
#include <XPLMUtilities.h>

enum FMCLed : unsigned char {
    BACKLIGHT = 0,
    SCREEN_BACKLIGHT = 1,
    OVERALL_LEDS_BRIGHTNESS = 2,

    _PFP_START = 3,
    PFP_CALL = 3,
    PFP_FAIL = 4,
    PFP_MSG = 5,
    PFP_OFST = 6,
    PFP_EXEC = 7,
    _PFP_END = 7,

    _MCDU_START = 8,
    MCDU_FAIL = 8,
    MCDU_FM = 9,
    MCDU_MCDU = 10,
    MCDU_MENU = 11,
    MCDU_FM1 = 12,
    MCDU_IND = 13,
    MCDU_RDY = 14,
    MCDU_STATUS = 15,
    MCDU_FM2 = 16,
    _MCDU_END = 16
};

struct FMCTextColor {
        enum : int {
            COLOR_BLACK = 0x0000,
            COLOR_AMBER = 0x0021,
            COLOR_WHITE = 0x0042,
            COLOR_CYAN = 0x0063,
            COLOR_GREEN = 0x0084,
            COLOR_MAGENTA = 0x00A5,
            COLOR_RED = 0x00C6,
            COLOR_YELLOW = 0x00E7,
            COLOR_DARKBROWN = 0x0108,
            COLOR_GREY = 0x0129,
            COLOR_LIGHTBROWN = 0x014A,
        };

        int value;

        constexpr FMCTextColor(int v) : value(v) {}

        constexpr operator int() const {
            return value;
        }

        // So it's FG x 33 + BG x 3 for background colors.. The / 11 is just the compressed form of / 33 * 3
        static constexpr FMCTextColor withBackgroundColor(FMCTextColor fg, FMCTextColor bg) {
            return {fg.value + (bg.value / 11)};
        }
};

struct FMCSpecialCharacter {
        static constexpr std::array<uint8_t, 3> WHITE_SQUARE = {0xE2, 0x96, 0xA1};          // U+25A1 WHITE SQUARE
        static constexpr std::array<uint8_t, 3> BLACK_SQUARE = {0xE2, 0x96, 0xA0};          // U+25A0 BLACK SQUARE
        static constexpr std::array<uint8_t, 3> OUTLINED_SQUARE = {0xE2, 0x98, 0x90};       // U+2610 BALLOT BOX
        static constexpr std::array<uint8_t, 3> FILLED_SQUARE = {0xE2, 0x96, 0x88};         // U+2588 FULL BLOCK
        static constexpr std::array<uint8_t, 3> FILLED_TRIANGLE_LEFT = {0xE2, 0x97, 0x80};  // U+25C0 BLACK LEFT-POINTING TRIANGLE
        static constexpr std::array<uint8_t, 3> FILLED_TRIANGLE_RIGHT = {0xE2, 0x96, 0xB6}; // U+25B6 BLACK RIGHT-POINTING TRIANGLE
        static constexpr std::array<uint8_t, 3> FILLED_TRIANGLE_UP = {0xE2, 0x96, 0xB2};    // U+25B2 BLACK UP-POINTING TRIANGLE
        static constexpr std::array<uint8_t, 3> FILLED_TRIANGLE_DOWN = {0xE2, 0x96, 0xBC};  // U+25BC BLACK DOWN-POINTING TRIANGLE
        static constexpr std::array<uint8_t, 3> ARROW_LEFT = {0xE2, 0x86, 0x90};            // U+2190 LEFTWARDS ARROW
        static constexpr std::array<uint8_t, 3> ARROW_RIGHT = {0xE2, 0x86, 0x92};           // U+2192 RIGHTWARDS ARROW
        static constexpr std::array<uint8_t, 3> ARROW_UP = {0xE2, 0x86, 0x91};              // U+2191 UPWARDS ARROW
        static constexpr std::array<uint8_t, 3> ARROW_DOWN = {0xE2, 0x86, 0x93};            // U+2193 DOWNWARDS ARROW
        static constexpr std::array<uint8_t, 2> DEGREES = {0xC2, 0xB0};                     // U+00B0 DEGREE SIGN
        static constexpr std::array<uint8_t, 2> TRIANGLE = {0xCE, 0x94};                    // U+0394 GREEK CAPITAL LETTER DELTA
        static constexpr std::array<uint8_t, 3> DIAMOND = {0xE2, 0xAC, 0xA1};               // U+2B21 WHITE HEXAGON
};

enum class FMCBackgroundVariant : unsigned char {
    GRAY = 1,
    BLACK,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    PURPLE,
    WINCTRL_LOGO
};

class ProductFMC;

class FMCAircraftProfile {
    protected:
        ProductFMC *product;

    public:
        FMCAircraftProfile(ProductFMC *product) :
            product(product) {};
        virtual ~FMCAircraftProfile() = default;

        virtual const std::vector<std::string> &displayDatarefs() const = 0;
        virtual const std::vector<FMCButtonDef> &buttonDefs() const = 0;
        virtual const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const = 0;
        virtual const std::map<char, FMCTextColor> &colorMap() const = 0;
        virtual void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) = 0;
        virtual void updatePage(std::vector<std::vector<char>> &page) = 0;
        virtual void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual bool shouldReadDatarefAsBytes(const std::string &dataref) const {
            return false;
        }
};

#endif

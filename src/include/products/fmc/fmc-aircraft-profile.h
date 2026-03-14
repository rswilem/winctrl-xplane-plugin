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

enum FMCTextColor : int {
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

    _ADD_BG = 0x1B,
    COLOR_AMBER_BG = COLOR_AMBER + _ADD_BG,
    COLOR_BLACK_BG = COLOR_BLACK + _ADD_BG,
    COLOR_WHITE_BG = COLOR_WHITE + _ADD_BG,
    COLOR_CYAN_BG = COLOR_CYAN + _ADD_BG,
    COLOR_GREEN_BG = COLOR_GREEN + _ADD_BG,
    COLOR_MAGENTA_BG = COLOR_MAGENTA + _ADD_BG,
    COLOR_RED_BG = COLOR_RED + _ADD_BG,
    COLOR_YELLOW_BG = COLOR_YELLOW + _ADD_BG,
    COLOR_DARKBROWN_BG = COLOR_DARKBROWN + _ADD_BG,
    COLOR_GREY_BG = COLOR_GREY + _ADD_BG,
    COLOR_LIGHTBROWN_BG = COLOR_LIGHTBROWN + _ADD_BG,
};

struct FMCSpecialCharacter {
        static constexpr std::array<uint8_t, 3> OUTLINED_SQUARE = {0xE2, 0x98, 0x90};    // U+2610
        static constexpr std::array<uint8_t, 3> FILLED_ARROW_LEFT = {0xE2, 0x97, 0x80};  // // U+25C0
        static constexpr std::array<uint8_t, 3> FILLED_ARROW_RIGHT = {0xE2, 0x96, 0xB6}; // U+25B6
        static constexpr std::array<uint8_t, 3> ARROW_LEFT = {0xE2, 0x86, 0x90};         // U+2190
        static constexpr std::array<uint8_t, 3> ARROW_RIGHT = {0xE2, 0x86, 0x92};        // U+2192
        static constexpr std::array<uint8_t, 3> ARROW_UP = {0xE2, 0x86, 0x91};           // U+2191
        static constexpr std::array<uint8_t, 3> ARROW_DOWN = {0xE2, 0x86, 0x93};         // U+2193
        static constexpr std::array<uint8_t, 2> DEGREES = {0xC2, 0xB0};                  // U+00B0
        static constexpr std::array<uint8_t, 2> TRIANGLE = {0xCE, 0x94};                 // U+0394
        static constexpr std::array<uint8_t, 3> DIAMOND = {0xE2, 0xAC, 0xA1};            // U+2B21
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

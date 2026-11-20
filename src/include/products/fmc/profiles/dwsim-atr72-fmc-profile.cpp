#include "dwsim-atr72-fmc-profile.h"

#include "dataref.h"
#include "logger.hpp"
#include "product-fmc.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

// DeltaWing Simulations ATR 72-600 Two Thales MCDUs: a captain unit drives
// MCDU 1, a first officer or observer unit MCDU 2. Every key is a phased command
// (DWSim/ATR_Series/commands/mcduN/<key>/toggle); a key held for more than 1 s counts as a second
// press in the aircraft, and CLR held for 1 s clears the whole scratchpad, so keys are held for as
// long as the hardware key.
//
// The ATR keys that the hardware does not have go to the keys closest in meaning:
//   ATR     MCDU                       PFP 3N              PFP 4 / PFP 7
//   DTO     DIR                        HOLD                HOLD
//   FPLN    F-PLN, SEC F-PLN           RTE, LEGS           RTE, LEGS
//   PERF    PERF                       DEP ARR, CLB        DEP ARR, ALTN (PFP 7)
//   VNAV    FUEL PRED                  CRZ                 VNAV
//   DATA    DATA, INIT                 INIT REF            INIT REF
//   MSG     ATC COMM                   DES                 ATC (PFP 4), FMC COMM
//   RMS     RAD NAV                    N1 LIMIT            NAV RAD
//   MRK     blank top right            FIX                 FIX
//   EXEC    OVFY                       EXEC                EXEC
//   CLR     CLR                        CLR, DEL            CLR, DEL
//   PREV    PREV PAGE, up arrow        PREV PAGE           PREV PAGE
//   NEXT    NEXT PAGE, down arrow      NEXT PAGE           NEXT PAGE
// The INIT page is reached from the DATA MENU (<INIT), and FPLN pressed again gives SEC FPLN,
// hence INIT -> DATA and SEC F-PLN -> FPLN. The ATR slash key is the "dash" command.
//
// Annunciators (DWSim/ATR_Series/lit/mcduN/*):
//   ATR      MCDU      PFP
//   FAIL     FAIL      FAIL
//   MSG      MCDU      MSG
//   EXEC     STATUS    EXEC
//   OFST     IND       OFST
//   POS      FM        -
//   MENU     MENU      CALL
//
// Screen: the aircraft publishes the text of both MCDU screens in the layout of the X-Plane CDU
// screen datarefs (FMS-MCDU Manual, SIM.2.2): 16 lines of 24 UTF-8 characters, line 0 the title,
// lines 1-12 the label/data pairs and line 14 the scratchpad, a style byte per character, a
// screen-on flag and the BRT level. The style byte follows X-Plane: bits 0-2 colour, 0x40 reverse
// video, 0x80 large font. The real screen has 14 lines, so line 14 goes to the hardware's
// scratchpad row 13.

namespace {
    constexpr const char *kCommandPrefix = "DWSim/ATR_Series/commands/";
    constexpr const char *kLitPrefix = "DWSim/ATR_Series/lit/";
    // Pedestal panel lighting, also the light of the MCDU keys
    constexpr const char *kPedestalBrightness = "DWSim/ATR_Series/lit/cockpit/mipPed/brightness";

    constexpr const char *kScreenPrefix = "DWSim/ATR_Series/mcdu";
    constexpr const char *kScreenTextLine = "/text_line";
    constexpr const char *kScreenStyleLine = "/style_line";
    constexpr const char *kScreenOn = "/screen_on";
    constexpr const char *kScreenBrightness = "/brightness";

    constexpr int kExportPageLines = 13; // title + six label/data pairs
    constexpr int kExportScratchpadLine = 14;

    constexpr unsigned char kStyleColorMask = 0x07;
    constexpr unsigned char kStyleReverse = 0x40;
    constexpr unsigned char kStyleLargeFont = 0x80;

    // Placeholders for the UTF-8 symbols, so that each character takes one byte (and lines up
    // with its style byte) until mapCharacter() turns it into the hardware glyph.
    enum Glyph : unsigned char {
        GLYPH_BOX = 0x01,
        GLYPH_DEGREE,
        GLYPH_ARROW_LEFT,
        GLYPH_ARROW_RIGHT,
        GLYPH_ARROW_UP,
        GLYPH_ARROW_DOWN,
        GLYPH_TRIANGLE_LEFT,
        GLYPH_TRIANGLE_RIGHT,
        GLYPH_DELTA,
    };

    const std::vector<std::pair<std::string, unsigned char>> &symbolPlaceholders() {
        static const std::vector<std::pair<std::string, unsigned char>> symbols = {
            {"□", GLYPH_BOX},
            {"☐", GLYPH_BOX},
            {"°", GLYPH_DEGREE},
            {"←", GLYPH_ARROW_LEFT},
            {"→", GLYPH_ARROW_RIGHT},
            {"↑", GLYPH_ARROW_UP},
            {"↓", GLYPH_ARROW_DOWN},
            {"◀", GLYPH_TRIANGLE_LEFT},
            {"▶", GLYPH_TRIANGLE_RIGHT},
            {"Δ", GLYPH_DELTA},
        };
        return symbols;
    }

    bool hasVisibleText(const std::string &text) {
        return std::any_of(text.begin(), text.end(), [](char c) {
            return c != ' ' && c != 0x00;
        });
    }
} // namespace

DWSimATR72FMCProfile::DWSimATR72FMCProfile(ProductFMC *product) :
    FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontAirbus);

    auto dataref = Dataref::getInstance();
    screenExportAvailable = dataref->exists(screenLineRef(kScreenTextLine, 0).c_str());
    if (!screenExportAvailable) {
        Logger::getInstance()->info("FMC: DWSim ATR 72-600 MCDU screen export not found (%s), keys only\n", screenLineRef(kScreenTextLine, 0).c_str());
    }

    dataref->monitorExistingDataref<float>(kPedestalBrightness, [product](float level) {
        product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(level, 0.0f, 1.0f) * 255));
    },
        this);

    dataref->monitorExistingDataref<bool>(screenRef(kScreenOn).c_str(), [this](bool) {
        updateScreenBacklight();
    },
        this);

    dataref->monitorExistingDataref<float>(screenRef(kScreenBrightness).c_str(), [this](float) {
        updateScreenBacklight();
    },
        this);

    const std::vector<std::pair<const char *, std::vector<FMCLed>>> annunciators = {
        {"fail", {FMCLed::MCDU_FAIL, FMCLed::PFP_FAIL}},
        {"msg", {FMCLed::MCDU_MCDU, FMCLed::PFP_MSG}},
        {"exec", {FMCLed::MCDU_STATUS, FMCLed::PFP_EXEC}},
        {"ofst", {FMCLed::MCDU_IND, FMCLed::PFP_OFST}},
        {"pos", {FMCLed::MCDU_FM}},
        {"request", {FMCLed::MCDU_MENU, FMCLed::PFP_CALL_DISPLAY}},
    };

    for (const auto &[name, leds] : annunciators) {
        // The lamp datarefs are light levels (0..1) that follow the BRT setting
        std::string ref = std::string(kLitPrefix) + mcdu() + "/" + name;
        dataref->monitorExistingDataref<bool>(ref.c_str(), [product, leds](bool lit) {
            for (FMCLed led : leds) {
                product->setLedBrightness(led, lit ? 1 : 0);
            }
        },
            this);
    }

    // The monitors only report changes, so start from a dark screen until the export says otherwise
    updateScreenBacklight();
}

bool DWSimATR72FMCProfile::IsEligible() {
    auto dataref = Dataref::getInstance();
    return dataref->get<std::string>("sim/aircraft/view/acf_ICAO") == "AT76" && dataref->exists("DWSim/ATR_Series/lit/mcdu1/exec");
}

std::string DWSimATR72FMCProfile::mcdu() const {
    return product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "mcdu1" : "mcdu2";
}

std::string DWSimATR72FMCProfile::screenRef(const char *suffix) const {
    return std::string(kScreenPrefix) + (product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2") + suffix;
}

std::string DWSimATR72FMCProfile::screenLineRef(const char *suffix, int line) const {
    return screenRef(suffix) + std::to_string(line);
}

const std::vector<std::string> &DWSimATR72FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    std::vector<std::string> refs;
    for (int line = 0; line <= kExportScratchpadLine; ++line) {
        refs.push_back(screenLineRef(kScreenTextLine, line));
        refs.push_back(screenLineRef(kScreenStyleLine, line));
    }
    refs.push_back(screenRef(kScreenOn));

    return cache.emplace(product->deviceVariant, std::move(refs)).first->second;
}

const std::vector<FMCButtonDef> &DWSimATR72FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const std::string p = std::string(kCommandPrefix) + mcdu() + "/";
    const std::string t = "/toggle";

    return cache.emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, p + "lsk1" + t},
                        {FMCKey::LSK2L, p + "lsk2" + t},
                        {FMCKey::LSK3L, p + "lsk3" + t},
                        {FMCKey::LSK4L, p + "lsk4" + t},
                        {FMCKey::LSK5L, p + "lsk5" + t},
                        {FMCKey::LSK6L, p + "lsk6" + t},
                        {FMCKey::LSK1R, p + "rsk1" + t},
                        {FMCKey::LSK2R, p + "rsk2" + t},
                        {FMCKey::LSK3R, p + "rsk3" + t},
                        {FMCKey::LSK4R, p + "rsk4" + t},
                        {FMCKey::LSK5R, p + "rsk5" + t},
                        {FMCKey::LSK6R, p + "rsk6" + t},

                        // Function keys
                        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_HOLD}, p + "dto" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::MCDU_SEC_FPLN, FMCKey::PFP_ROUTE, FMCKey::PFP_LEGS}, p + "fpln" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP_DEP_ARR, FMCKey::PFP3_CLB, FMCKey::PFP7_ALTN}, p + "perf" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_FUEL_PRED, FMCKey::PFP3_CRZ, FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, p + "vnav" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::MCDU_INIT, FMCKey::PFP_INIT_REF}, p + "data" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP3_DES, FMCKey::PFP4_ATC, FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, p + "msg" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP3_N1_LIMIT, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, p + "rms" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_EMPTY_TOP_RIGHT, FMCKey::PFP_FIX}, p + "mrk" + t},
                        {FMCKey::PROG, p + "prog" + t},

                        // Access keys
                        {FMCKey::MENU, p + "menu" + t},
                        {std::vector<FMCKey>{FMCKey::PAGE_PREV, FMCKey::MCDU_PAGE_UP}, p + "prev" + t},
                        {std::vector<FMCKey>{FMCKey::PAGE_NEXT, FMCKey::MCDU_PAGE_DOWN}, p + "next" + t},
                        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_EXEC}, p + "exec" + t},
                        {FMCKey::BRIGHTNESS_UP, p + "brtUp" + t},
                        {FMCKey::BRIGHTNESS_DOWN, p + "brtDn" + t},

                        // Alphanumeric keys
                        {std::vector<FMCKey>{FMCKey::CLR, FMCKey::PFP_DEL}, p + "clr" + t},
                        {FMCKey::SLASH, p + "dash" + t},
                        {FMCKey::PERIOD, p + "dot" + t},
                        {FMCKey::PLUSMINUS, p + "pluMinus" + t},
                        {FMCKey::SPACE, p + "space" + t},
                        {FMCKey::KEY0, p + "_0" + t},
                        {FMCKey::KEY1, p + "_1" + t},
                        {FMCKey::KEY2, p + "_2" + t},
                        {FMCKey::KEY3, p + "_3" + t},
                        {FMCKey::KEY4, p + "_4" + t},
                        {FMCKey::KEY5, p + "_5" + t},
                        {FMCKey::KEY6, p + "_6" + t},
                        {FMCKey::KEY7, p + "_7" + t},
                        {FMCKey::KEY8, p + "_8" + t},
                        {FMCKey::KEY9, p + "_9" + t},
                        {FMCKey::KEYA, p + "a" + t},
                        {FMCKey::KEYB, p + "b" + t},
                        {FMCKey::KEYC, p + "c" + t},
                        {FMCKey::KEYD, p + "d" + t},
                        {FMCKey::KEYE, p + "e" + t},
                        {FMCKey::KEYF, p + "f" + t},
                        {FMCKey::KEYG, p + "g" + t},
                        {FMCKey::KEYH, p + "h" + t},
                        {FMCKey::KEYI, p + "i" + t},
                        {FMCKey::KEYJ, p + "j" + t},
                        {FMCKey::KEYK, p + "k" + t},
                        {FMCKey::KEYL, p + "l" + t},
                        {FMCKey::KEYM, p + "m" + t},
                        {FMCKey::KEYN, p + "n" + t},
                        {FMCKey::KEYO, p + "o" + t},
                        {FMCKey::KEYP, p + "p" + t},
                        {FMCKey::KEYQ, p + "q" + t},
                        {FMCKey::KEYR, p + "r" + t},
                        {FMCKey::KEYS, p + "s" + t},
                        {FMCKey::KEYT, p + "t" + t},
                        {FMCKey::KEYU, p + "u" + t},
                        {FMCKey::KEYV, p + "v" + t},
                        {FMCKey::KEYW, p + "w" + t},
                        {FMCKey::KEYX, p + "x" + t},
                        {FMCKey::KEYY, p + "y" + t},
                        {FMCKey::KEYZ, p + "z" + t},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &DWSimATR72FMCProfile::buttonKeyMap() const {
    static std::unordered_map<FMCDeviceVariant, std::unordered_map<FMCKey, const FMCButtonDef *>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        std::unordered_map<FMCKey, const FMCButtonDef *> map;
        for (const auto &button : buttonDefs()) {
            std::visit([&](auto &&key) {
                using T = std::decay_t<decltype(key)>;
                if constexpr (std::is_same_v<T, FMCKey>) {
                    map[key] = &button;
                } else {
                    for (FMCKey k : key) {
                        map[k] = &button;
                    }
                }
            },
                button.key);
        }
        it = cache.emplace(product->deviceVariant, std::move(map)).first;
    }
    return it->second;
}

const std::map<char, FMCTextColor> &DWSimATR72FMCProfile::colorMap() const {
    // Key: style colour (bits 0-2), plus kStyleReverse for reverse video (black on the colour)
    static const std::map<char, FMCTextColor> colors = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x02, FMCTextColor::COLOR_RED},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_AMBER},
        {0x07, FMCTextColor::COLOR_WHITE},
        {0x40, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},
        {0x41, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_CYAN)},
        {0x42, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_RED)},
        {0x43, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_YELLOW)},
        {0x44, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_GREEN)},
        {0x45, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_MAGENTA)},
        {0x46, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_AMBER)},
        {0x47, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},
    };
    return colors;
}

void DWSimATR72FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    auto append = [buffer](const auto &glyph) {
        buffer->insert(buffer->end(), glyph.begin(), glyph.end());
    };

    switch (character) {
        case GLYPH_BOX:
            append(FMCSpecialCharacter::OUTLINED_SQUARE);
            break;

        case GLYPH_DEGREE:
            append(FMCSpecialCharacter::DEGREES);
            break;

        case GLYPH_ARROW_LEFT:
            append(FMCSpecialCharacter::ARROW_LEFT);
            break;

        case GLYPH_ARROW_RIGHT:
            append(FMCSpecialCharacter::ARROW_RIGHT);
            break;

        case GLYPH_ARROW_UP:
            append(FMCSpecialCharacter::ARROW_UP);
            break;

        case GLYPH_ARROW_DOWN:
            append(FMCSpecialCharacter::ARROW_DOWN);
            break;

        case GLYPH_TRIANGLE_LEFT:
            append(FMCSpecialCharacter::FILLED_TRIANGLE_LEFT);
            break;

        case GLYPH_TRIANGLE_RIGHT:
            append(FMCSpecialCharacter::FILLED_TRIANGLE_RIGHT);
            break;

        case GLYPH_DELTA:
            append(FMCSpecialCharacter::TRIANGLE);
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void DWSimATR72FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto dataref = Dataref::getInstance();
    if (!screenExportAvailable || !dataref->getCached<bool>(screenRef(kScreenOn).c_str())) {
        return;
    }

    auto drawLine = [&](int exportLine, int row) -> bool {
        std::string text = dataref->getCached<std::string>(screenLineRef(kScreenTextLine, exportLine).c_str());
        if (!hasVisibleText(text)) {
            return false;
        }

        std::vector<unsigned char> styles = dataref->getCached<std::vector<unsigned char>>(screenLineRef(kScreenStyleLine, exportLine).c_str());

        for (const auto &[symbol, placeholder] : symbolPlaceholders()) {
            size_t pos = 0;
            while ((pos = text.find(symbol, pos)) != std::string::npos) {
                text.replace(pos, symbol.length(), 1, static_cast<char>(placeholder));
                pos += 1;
            }
        }

        for (char &c : text) {
            if (static_cast<unsigned char>(c) > 127) {
                c = '?';
            }
        }

        for (int i = 0; i < static_cast<int>(text.size()) && i < static_cast<int>(ProductFMC::PageCharsPerLine); ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            unsigned char style = i < static_cast<int>(styles.size()) ? styles[i] : 0x00;
            char color = static_cast<char>((style & kStyleColorMask) | (style & kStyleReverse));
            bool fontSmall = (style & kStyleLargeFont) == 0;
            product->writeLineToPage(page, row, i, std::string(1, c), color, fontSmall);
        }

        return true;
    };

    for (int line = 0; line < kExportPageLines; ++line) {
        drawLine(line, line);
    }

    // Line 14 is the scratchpad; fall back to line 13 in case the export packs the 14 lines
    if (!drawLine(kExportScratchpadLine, ProductFMC::PageLines - 1)) {
        drawLine(kExportPageLines, ProductFMC::PageLines - 1);
    }
}

void DWSimATR72FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

void DWSimATR72FMCProfile::updateScreenBacklight() {
    auto dataref = Dataref::getInstance();
    float level = 0.0f;
    if (screenExportAvailable && dataref->getCached<bool>(screenRef(kScreenOn).c_str())) {
        const std::string brightnessRef = screenRef(kScreenBrightness);
        level = dataref->exists(brightnessRef.c_str()) ? std::clamp(dataref->getCached<float>(brightnessRef.c_str()), 0.0f, 1.0f) : 1.0f;
    }

    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, static_cast<uint8_t>(level * 255));
}

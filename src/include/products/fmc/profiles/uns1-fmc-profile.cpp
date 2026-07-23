#include "uns1-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

UNS1FMCProfile::UNS1FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);

    auto font = Font::GlyphData("UNS1.xpwwf", product->identifierByte, product->hardwareType);
    if (!font.empty()) {
        for (auto &packet : font) {
            product->writeData(packet);
        }
    } else {
        product->setFont(FontVariant::Default);
    }
}

std::string UNS1FMCProfile::displayPathPrefix() const {
    return "uns1/cdu1";
}

std::string UNS1FMCProfile::commandPathPrefix() const {
    return "uns1/fms1";
}

const std::vector<std::string> &UNS1FMCProfile::displayDatarefs() const {
    if (!cachedDisplayDatarefs) {
        const std::string prefix = displayPathPrefix();
        std::vector<std::string> refs;
        const int lines = screenLineCount();
        for (int i = 0; i < lines; ++i) {
            refs.push_back(prefix + "/text_line_" + std::to_string(i));
            refs.push_back(prefix + "/style_line_" + std::to_string(i));
        }
        cachedDisplayDatarefs = std::move(refs);
    }
    return *cachedDisplayDatarefs;
}

const std::vector<FMCButtonDef> &UNS1FMCProfile::buttonDefs() const {
    if (!cachedButtonDefs) {
        const std::string fms = commandPathPrefix();

        std::vector<FMCButtonDef> buttons = {
            {FMCKey::LSK1L, fms + "/lsk_l1", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK2L, fms + "/lsk_l2", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK3L, fms + "/lsk_l3", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK4L, fms + "/lsk_l4", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK5L, fms + "/lsk_l5", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK1R, fms + "/lsk_r1", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK2R, fms + "/lsk_r2", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK3R, fms + "/lsk_r3", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK4R, fms + "/lsk_r4", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::LSK5R, fms + "/lsk_r5", FMCDatarefType::EXECUTE_CMD_ONCE},

            // UNS-1 only has 5 LSKs per side — LSK6 mapped empty
            {FMCKey::LSK6L, ""},
            {FMCKey::LSK6R, ""},

            {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_INIT_REF}, fms + "/dto", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, fms + "/perf", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MCDU_DATA, fms + "/data", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MCDU_FPLN, fms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::PFP_LEGS, fms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::PFP_ROUTE, fms + "/fpl", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, fms + "/nav", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MCDU_FUEL_PRED, fms + "/fuel", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, fms + "/tune", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, fms + "/vnav", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, fms + "/list", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MENU, fms + "/menu", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::PFP_EXEC, fms + "/key_enter", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MCDU_OVERFLY, fms + "/key_enter", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::MCDU_EMPTY_TOP_RIGHT, fms + "/pwr", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::CLR, FMCKey::PFP_DEL}, fms + "/key_back", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY0, fms + "/key_0", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY1, fms + "/key_1", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY2, fms + "/key_2", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY3, fms + "/key_3", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY4, fms + "/key_4", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY5, fms + "/key_5", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY6, fms + "/key_6", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY7, fms + "/key_7", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY8, fms + "/key_8", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEY9, fms + "/key_9", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYA, fms + "/key_A", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYB, fms + "/key_B", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYC, fms + "/key_C", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYD, fms + "/key_D", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYE, fms + "/key_E", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYF, fms + "/key_F", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYG, fms + "/key_G", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYH, fms + "/key_H", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYI, fms + "/key_I", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYJ, fms + "/key_J", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYK, fms + "/key_K", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYL, fms + "/key_L", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYM, fms + "/key_M", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYN, fms + "/key_N", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYO, fms + "/key_O", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYP, fms + "/key_P", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYQ, fms + "/key_Q", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYR, fms + "/key_R", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYS, fms + "/key_S", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYT, fms + "/key_T", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYU, fms + "/key_U", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYV, fms + "/key_V", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYW, fms + "/key_W", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYX, fms + "/key_X", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYY, fms + "/key_Y", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::KEYZ, fms + "/key_Z", FMCDatarefType::EXECUTE_CMD_ONCE},
            {FMCKey::PLUSMINUS, fms + "/plusminus", FMCDatarefType::EXECUTE_CMD_ONCE},

            // PAGE_PREV/PAGE_NEXT are the physical keys on real UNS-1-shaped
            // (PFP3N/PFP4/PFP7) hardware; MCDU_PAGE_UP/MCDU_PAGE_DOWN are the
            // equivalent on MCDU-shaped hardware, which has no PAGE_PREV/NEXT
            // indices of its own. Alias both onto the same commands so either
            // hardware shape works.
            {std::vector<FMCKey>{FMCKey::MCDU_PAGE_UP, FMCKey::PAGE_PREV}, fms + "/prev", FMCDatarefType::EXECUTE_CMD_ONCE},
            {std::vector<FMCKey>{FMCKey::MCDU_PAGE_DOWN, FMCKey::PAGE_NEXT}, fms + "/next", FMCDatarefType::EXECUTE_CMD_ONCE},

            // Remaining keys — no UNS-1 equivalent
            {FMCKey::BRIGHTNESS_UP, ""},
            {FMCKey::BRIGHTNESS_DOWN, ""},
            {FMCKey::MCDU_EMPTY_BOTTOM_LEFT, ""},
            {FMCKey::MCDU_SEC_FPLN, ""},
            {FMCKey::SLASH, ""},
            {FMCKey::PERIOD, ""},
            {FMCKey::SPACE, ""},
            {FMCKey::PROG, ""},
            {FMCKey::PFP_HOLD, ""},
            {FMCKey::PFP_FIX, ""},
            {FMCKey::PFP3_CLB, ""},
            {FMCKey::PFP3_CRZ, ""},
            {FMCKey::PFP3_DES, ""},
            {FMCKey::PFP4_FMC_COMM, ""},
            {FMCKey::PFP7_ALTN, ""},
            {FMCKey::PFP7_FMC_COMM, ""},
        };

        cachedButtonDefs = std::move(buttons);
    }
    return *cachedButtonDefs;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &UNS1FMCProfile::buttonKeyMap() const {
    if (!cachedButtonKeyMap) {
        std::unordered_map<FMCKey, const FMCButtonDef *> map;
        const auto &buttons = buttonDefs();
        for (const auto &button : buttons) {
            std::visit([&](auto &&k) {
                using T = std::decay_t<decltype(k)>;
                if constexpr (std::is_same_v<T, FMCKey>) {
                    map[k] = &button;
                } else {
                    for (const auto &key : k) {
                        map[key] = &button;
                    }
                }
            },
                button.key);
        }
        cachedButtonKeyMap = std::move(map);
    }
    return *cachedButtonKeyMap;
}

const std::map<char, FMCTextColor> &UNS1FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_WHITE},
        {0x02, FMCTextColor::COLOR_GREEN},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_GREEN},
        {0x07, FMCTextColor::COLOR_CYAN},
        {0x0B, FMCTextColor::COLOR_GREEN},
        {0x40, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},  // inverted (ACCEPT)
    };
    return colMap;
}

void UNS1FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '<':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case '>':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 30:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 31:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case '^':
            buffer->insert(buffer->end(), FMCSpecialCharacter::TRIANGLE.begin(), FMCSpecialCharacter::TRIANGLE.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void UNS1FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string prefix = displayPathPrefix();

    // Replace unicode symbols with single-byte placeholders
    const std::vector<std::pair<std::string, unsigned char>> symbols = {
        {"\u2190", '<'},  {"\u2192", '>'},  {"\u2191", 30},  {"\u2193", 31},
        {"\u2610", '#'},  {"\u00B0", '`'},  {"\u0394", '^'},
        {"\u2194", '<'},  {"\u2196", '<'},  {"\u2197", '>'},  {"\u2198", '>'},  {"\u2199", '<'},
        {"\u21E6", '<'},  {"\u21E8", '>'},  {"\u21E7", 30},  {"\u21E9", 31},
        {"\u2500", '-'},  {"\u2502", '|'},  {"\u250C", '+'}, {"\u2510", '+'}, {"\u2514", '+'}, {"\u2518", '+'},
        {"\u251C", '|'},  {"\u2524", '|'},  {"\u252C", '+'}, {"\u2534", '+'}, {"\u253C", '+'},
        {"\u2550", '='},  {"\u2551", '|'},  {"\u2554", '+'}, {"\u2557", '+'}, {"\u255A", '+'}, {"\u255D", '+'},
        {"\u2560", '+'},  {"\u2563", '+'},  {"\u2566", '+'}, {"\u2569", '+'}, {"\u256C", '+'},
        {"\u256D", '|'},  {"\u256E", '|'},  {"\u256F", '|'},  {"\u2570", '|'},
        {"\u23A1", '+'},  {"\u23A4", '+'},  {"\u23A7", '{'},  {"\u23AB", '}'},
        {"\u27E6", '['},  {"\u27E7", ']'},
    };

    const int lines = screenLineCount();

    for (int lineNum = 0; lineNum < lines; ++lineNum) {
        std::string textDataref = prefix + "/text_line_" + std::to_string(lineNum);
        std::string styleDataref = prefix + "/style_line_" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        for (const auto &symbol : symbols) {
            size_t pos = 0;
            while ((pos = text.find(symbol.first, pos)) != std::string::npos) {
                text.replace(pos, symbol.first.length(), std::string(1, static_cast<char>(symbol.second)));
                pos += 1;
            }
        }

        // Map remaining high-byte characters to ASCII equivalents
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c <= 127) { continue; }
            switch (c) {
                case 0xB0: case 0xB1: case 0xB2: text[i] = ' '; break;
                case 0xB3: case 0xDD: case 0xDE: text[i] = '|'; break;
                case 0xC4: case 0xDC: case 0xDF: text[i] = '-'; break;
                case 0xBA:                     text[i] = '|'; break;
                case 0xCD:                     text[i] = '='; break;
                case 0xBF: case 0xC0: case 0xD9: case 0xDA:
                case 0xB4: case 0xC3: case 0xC1: case 0xC2: case 0xC5:
                case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9:
                case 0xBB: case 0xBC: case 0xBD: case 0xBE:
                case 0xC6: case 0xC7: case 0xC8: case 0xC9:
                case 0xCA: case 0xCB: case 0xCC: case 0xCE: case 0xCF:
                case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4:
                case 0xD5: case 0xD6: case 0xD7: case 0xD8:
                    text[i] = '+'; break;
                case 0xDB: text[i] = '#'; break;
                default:   text[i] = ' '; break;
            }
        }

        int displayLine = lineNum;
        if (displayLine >= ProductFMC::PageLines) {
            break;
        }

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            bool fontSmall = false;
            unsigned char styleByte = (i < styleBytes.size()) ? styleBytes[i] : 0x00;
            fontSmall = (styleByte & 0xF0) == 0x00;

            bool isInverted = (styleByte & 0x40);
            if (c == ' ' && !isInverted) {
                continue;
            }

            unsigned char colorIdx = styleByte & 0x0F;
            if (styleByte & 0x40) {
                colorIdx = 0x40;  // inverted/reverse video
            }

            product->writeLineToPage(page, displayLine, i, std::string(1, c), colorIdx, fontSmall);
        }
    }
}

void UNS1FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == FMCDatarefType::SET_VALUE || button->datarefType == FMCDatarefType::SET_VALUE_PHASED) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        if (button->datarefType == FMCDatarefType::SET_VALUE && phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<double>(button->dataref.c_str(), phase == xplm_CommandBegin ? value : 0.0);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::ADJUST_VALUE) {
        double currentValue = datarefManager->get<double>(button->dataref.c_str());
        datarefManager->set<double>(button->dataref.c_str(), currentValue + button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_MULTIPLE_CMD_ONCE) {
        std::stringstream ss(button->dataref);
        std::string item;
        std::vector<std::string> commands;
        while (std::getline(ss, item, ',')) {
            commands.push_back(item);
        }

        for (const auto &cmd : commands) {
            datarefManager->executeCommand(cmd.c_str());
        }
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

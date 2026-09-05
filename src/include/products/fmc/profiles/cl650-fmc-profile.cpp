#include "cl650-fmc-profile.h"

#include "config.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

CL650FMCProfile::CL650FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);

    product->setFont(FontVariant::FontAirbus);

    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";
    const std::string screenBrtRef = "CL650/CDU/" + cdu + "/screen/brt";
    const std::string brtRef = "CL650/lamps/integ/1A1FS_cdu" + cdu;

    Dataref::getInstance()->monitorExistingDataref<float>(screenBrtRef.c_str(), [product](float val) {
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255));
    });

    Dataref::getInstance()->monitorExistingDataref<float>(brtRef.c_str(), [product](float val) {
        product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255));
    });

#ifdef DEBUG
    Dataref::getInstance()->createCommand(
        PRODUCT_NAME "/debug/cl650_dump_styles", "Dump CL650 style_line values to log", [this](XPLMCommandPhase inPhase) {
            if (inPhase != xplm_CommandBegin) {
                return;
            }

            auto datarefManager = Dataref::getInstance();
            const std::string cdu = this->product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";

            Logger::getInstance()->info("=== CL650 CDU style_line dump (CDU %s) ===\n", cdu.c_str());

            for (int i = 0; i < 15; ++i) {
                std::string styleDataref = "CL650/CDU/" + cdu + "/screen/style_line" + std::to_string(i);
                std::string textDataref = "CL650/CDU/" + cdu + "/screen/text_line" + std::to_string(i);

                std::vector<unsigned char> styleBytes = datarefManager->get<std::vector<unsigned char>>(styleDataref.c_str());
                std::string text = datarefManager->get<std::string>(textDataref.c_str());

                // Take first non-space char as reference
                char sample = ' ';
                for (char c : text) {
                    if (c != ' ' && c != '\0') { sample = c; break; }
                }

                std::stringstream hexStyles;
                hexStyles << std::hex << std::setfill('0');
                for (int j = 0; j < 24 && j < styleBytes.size(); ++j) {
                    if (j > 0) hexStyles << " ";
                    hexStyles << std::setw(2) << (int)styleBytes[j];
                }

                Logger::getInstance()->info("  line%02d sample='%c' styles=[%s]\n", i, sample, hexStyles.str().c_str());
            }

            Logger::getInstance()->info("=== End CL650 style_line dump ===\n");
        });
#endif
}

CL650FMCProfile::~CL650FMCProfile() {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";
    Dataref::getInstance()->unbind(("CL650/CDU/" + cdu + "/screen/brt").c_str());
    Dataref::getInstance()->unbind(("CL650/lamps/integ/1A1FS_cdu" + cdu).c_str());
}

bool CL650FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("CL650/CDU/1/screen/text_line0");
}

const std::vector<std::string> &CL650FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";
        std::vector<std::string> refs;
        refs.push_back("CL650/CDU/" + cdu + "/screen/brt");
        static constexpr int ScreenLines = 15;
        for (int i = 0; i < ScreenLines; ++i) {
            refs.push_back("CL650/CDU/" + cdu + "/screen/text_line" + std::to_string(i));
            refs.push_back("CL650/CDU/" + cdu + "/screen/style_line" + std::to_string(i));
        }
        it = cache.emplace(product->deviceVariant, std::move(refs)).first;
    }
    return it->second;
}

const std::vector<FMCButtonDef> &CL650FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";

        std::vector<FMCButtonDef> buttons = {
            {FMCKey::LSK1L, "CL650/CDU/" + cdu + "/lsk_l1", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK2L, "CL650/CDU/" + cdu + "/lsk_l2", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK3L, "CL650/CDU/" + cdu + "/lsk_l3", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK4L, "CL650/CDU/" + cdu + "/lsk_l4", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK5L, "CL650/CDU/" + cdu + "/lsk_l5", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK6L, "CL650/CDU/" + cdu + "/lsk_l6", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK1R, "CL650/CDU/" + cdu + "/lsk_r1", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK2R, "CL650/CDU/" + cdu + "/lsk_r2", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK3R, "CL650/CDU/" + cdu + "/lsk_r3", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK4R, "CL650/CDU/" + cdu + "/lsk_r4", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK5R, "CL650/CDU/" + cdu + "/lsk_r5", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::LSK6R, "CL650/CDU/" + cdu + "/lsk_r6", FMCDatarefType::SET_VALUE, 1.0},

            {FMCKey::MCDU_DIR, "CL650/CDU/" + cdu + "/dir", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "CL650/CDU/" + cdu + "/idx", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, "CL650/CDU/" + cdu + "/perf", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::MCDU_DATA, "CL650/CDU/" + cdu + "/mfd_data", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::MCDU_SEC_FPLN, FMCKey::PFP_ROUTE}, "CL650/CDU/" + cdu + "/fpln", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "CL650/CDU/" + cdu + "/tun", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN}, "CL650/CDU/" + cdu + "/legs", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "CL650/CDU/" + cdu + "/dep_arr", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "CL650/CDU/" + cdu + "/exec", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::CLR, FMCKey::PFP_DEL}, "CL650/CDU/" + cdu + "/clr_del", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::MENU, "CL650/CDU/" + cdu + "/dspl_menu", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PAGE_NEXT, FMCKey::MCDU_PAGE_UP}, "CL650/CDU/" + cdu + "/next", FMCDatarefType::SET_VALUE, 1.0},
            {std::vector<FMCKey>{FMCKey::PAGE_PREV, FMCKey::MCDU_PAGE_DOWN}, "CL650/CDU/" + cdu + "/prev", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::MCDU_OVERFLY, "CL650/CDU/" + cdu + "/mfd_adv", FMCDatarefType::SET_VALUE, 1.0},

            {FMCKey::BRIGHTNESS_UP, "CL650/CDU/" + cdu + "/brt_up"},
            {FMCKey::BRIGHTNESS_DOWN, "CL650/CDU/" + cdu + "/brt_down"},

            {FMCKey::KEY1, "CL650/CDU/" + cdu + "/char_1", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY2, "CL650/CDU/" + cdu + "/char_2", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY3, "CL650/CDU/" + cdu + "/char_3", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY4, "CL650/CDU/" + cdu + "/char_4", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY5, "CL650/CDU/" + cdu + "/char_5", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY6, "CL650/CDU/" + cdu + "/char_6", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY7, "CL650/CDU/" + cdu + "/char_7", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY8, "CL650/CDU/" + cdu + "/char_8", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY9, "CL650/CDU/" + cdu + "/char_9", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEY0, "CL650/CDU/" + cdu + "/char_0", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::PERIOD, "CL650/CDU/" + cdu + "/char_period", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::PLUSMINUS, "CL650/CDU/" + cdu + "/char_plus_minus", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::SLASH, "CL650/CDU/" + cdu + "/char_slash", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::SPACE, "CL650/CDU/" + cdu + "/char_space", FMCDatarefType::SET_VALUE, 1.0},

            {FMCKey::KEYA, "CL650/CDU/" + cdu + "/char_A", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYB, "CL650/CDU/" + cdu + "/char_B", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYC, "CL650/CDU/" + cdu + "/char_C", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYD, "CL650/CDU/" + cdu + "/char_D", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYE, "CL650/CDU/" + cdu + "/char_E", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYF, "CL650/CDU/" + cdu + "/char_F", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYG, "CL650/CDU/" + cdu + "/char_G", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYH, "CL650/CDU/" + cdu + "/char_H", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYI, "CL650/CDU/" + cdu + "/char_I", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYJ, "CL650/CDU/" + cdu + "/char_J", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYK, "CL650/CDU/" + cdu + "/char_K", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYL, "CL650/CDU/" + cdu + "/char_L", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYM, "CL650/CDU/" + cdu + "/char_M", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYN, "CL650/CDU/" + cdu + "/char_N", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYO, "CL650/CDU/" + cdu + "/char_O", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYP, "CL650/CDU/" + cdu + "/char_P", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYQ, "CL650/CDU/" + cdu + "/char_Q", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYR, "CL650/CDU/" + cdu + "/char_R", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYS, "CL650/CDU/" + cdu + "/char_S", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYT, "CL650/CDU/" + cdu + "/char_T", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYU, "CL650/CDU/" + cdu + "/char_U", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYV, "CL650/CDU/" + cdu + "/char_V", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYW, "CL650/CDU/" + cdu + "/char_W", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYX, "CL650/CDU/" + cdu + "/char_X", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYY, "CL650/CDU/" + cdu + "/char_Y", FMCDatarefType::SET_VALUE, 1.0},
            {FMCKey::KEYZ, "CL650/CDU/" + cdu + "/char_Z", FMCDatarefType::SET_VALUE, 1.0},

            // Unmapped — no CL650 CDU equivalent
            {FMCKey::PROG, ""},
            {FMCKey::MCDU_EMPTY_BOTTOM_LEFT, ""},
            {FMCKey::MCDU_FUEL_PRED, ""},
            {FMCKey::MCDU_ATC_COMM, ""},
            {FMCKey::MCDU_AIRPORT, ""},
            {FMCKey::PFP_HOLD, ""},
            {FMCKey::PFP_FIX, ""},
            {FMCKey::PFP3_CLB, ""},
            {FMCKey::PFP3_CRZ, ""},
            {FMCKey::PFP3_DES, ""},
            {FMCKey::PFP4_ATC, ""},
            {FMCKey::PFP4_VNAV, ""},
            {FMCKey::PFP4_FMC_COMM, ""},
            {FMCKey::PFP7_ALTN, ""},
            {FMCKey::PFP7_VNAV, ""},
            {FMCKey::PFP7_FMC_COMM, ""},
        };

        it = cache.emplace(product->deviceVariant, std::move(buttons)).first;
    }
    return it->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &CL650FMCProfile::buttonKeyMap() const {
    static std::unordered_map<FMCDeviceVariant, std::unordered_map<FMCKey, const FMCButtonDef *>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
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
        it = cache.emplace(product->deviceVariant, std::move(map)).first;
    }
    return it->second;
}

const std::map<char, FMCTextColor> &CL650FMCProfile::colorMap() const {
    // CL650 style byte: low nibble = colour index, 0x80 = large font. 0x07 is default/empty,
    // left unmapped so it falls through to COLOR_WHITE.
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x02, FMCTextColor::COLOR_GREEN},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_AMBER},
    };
    return colMap;
}

void CL650FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
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

void CL650FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";

    // Replace unicode symbols with single-byte placeholders
    const std::vector<std::pair<std::string, unsigned char>> symbols = {
        {"\u2190", '<'},  // ← left arrow
        {"\u2192", '>'},  // → right arrow
        {"\u2191", 30},   // ↑ up arrow
        {"\u2193", 31},   // ↓ down arrow
        {"\u2610", '#'},  // ☐ ballot box
        {"\u00B0", '`'},  // ° degree
        {"\u0394", '^'},  // Δ delta (looks like triangle)
        // Additional arrows
        {"\u2194", '<'},  // ↔ left-right
        {"\u2196", '<'},  // ↖ NW
        {"\u2197", '>'},  // ↗ NE
        {"\u2198", '>'},  // ↘ SE
        {"\u2199", '<'},  // ↙ SW
        {"\u21E6", '<'},  // ⇦ thinner left
        {"\u21E8", '>'},  // ⇨ thinner right
        {"\u21E7", 30},   // ⇧ thinner up
        {"\u21E9", 31},   // ⇩ thinner down
        // Box-drawing (light)
        {"\u2500", '-'},  // ─ horizontal
        {"\u2502", '|'},  // │ vertical
        {"\u250C", '+'},  // ┌ down-right
        {"\u2510", '+'},  // ┐ down-left
        {"\u2514", '+'},  // └ up-right
        {"\u2518", '+'},  // ┘ up-left
        {"\u251C", '|'},  // ├ vertical-right (T-junction left)
        {"\u2524", '|'},  // ┤ vertical-left (T-junction right)
        {"\u252C", '+'},  // ┬ down-horizontal
        {"\u2534", '+'},  // ┴ up-horizontal
        {"\u253C", '+'},  // ┼ vertical-horizontal
        // Box-drawing (heavy)
        {"\u2550", '='},  // ═ double horizontal
        {"\u2551", '|'},  // ║ double vertical
        {"\u2554", '+'},  // ╔ double down-right
        {"\u2557", '+'},  // ╗ double down-left
        {"\u255A", '+'},  // ╚ double up-right
        {"\u255D", '+'},  // ╝ double up-left
        {"\u2560", '+'},  // ╠ double vertical-right
        {"\u2563", '+'},  // ╣ double vertical-left
        {"\u2566", '+'},  // ╦ double down-horizontal
        {"\u2569", '+'},  // ╩ double up-horizontal
        {"\u256C", '+'},  // ╬ double vertical-horizontal
        // Arcs / rounded corners
        {"\u256D", '|'},  // ╭ arc down-right
        {"\u256E", '|'},  // ╮ arc down-left
        {"\u256F", '|'},  // ╯ arc up-left
        {"\u2570", '|'},  // ╰ arc up-right
        // Brackets / corners
        {"\u23A1", '+'},  // ⎡
        {"\u23A4", '+'},  // ⎤
        {"\u23A7", '{'},  // ⎧ curly
        {"\u23AB", '}'},  // ⎫ curly
        {"\u27E6", '['},  // ⟦
        {"\u27E7", ']'},  // ⟧
    };

    static constexpr int ScreenLines = 14; // text_line0..text_line13; line 14 is a message line we skip

    for (int lineNum = 0; lineNum < ScreenLines; ++lineNum) {
        std::string textDataref = "CL650/CDU/" + cdu + "/screen/text_line" + std::to_string(lineNum);
        std::string styleDataref = "CL650/CDU/" + cdu + "/screen/style_line" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        // style_lineN displayed as int[24] in DataRefEditor but stored as byte array
        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        // Replace unicode symbols with single-byte placeholders
        for (const auto &symbol : symbols) {
            size_t pos = 0;
            while ((pos = text.find(symbol.first, pos)) != std::string::npos) {
                text.replace(pos, symbol.first.length(), std::string(1, static_cast<char>(symbol.second)));
                pos += 1;
            }
        }

        // Map remaining high-byte characters (CP437 box-drawing, etc.) to ASCII equivalents
        for (size_t i = 0; i < text.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c <= 127) { continue; }
            switch (c) {
                // CP437 box-drawing
                case 0xB0: case 0xB1: case 0xB2: text[i] = ' '; break;
                case 0xB3: case 0xDD: case 0xDE: text[i] = '|'; break;
                case 0xC4: case 0xDC: case 0xDF: text[i] = '-'; break;
                case 0xBA:                     text[i] = '|'; break;
                case 0xCD:                     text[i] = '='; break;
                // CP437 single-line box corners + junctions → +
                case 0xBF: case 0xC0: case 0xD9: case 0xDA:
                case 0xB4: case 0xC3: case 0xC1: case 0xC2: case 0xC5:
                // CP437 double-line and mixed corners → +
                case 0xB5: case 0xB6: case 0xB7: case 0xB8: case 0xB9:
                case 0xBB: case 0xBC: case 0xBD: case 0xBE:
                case 0xC6: case 0xC7: case 0xC8: case 0xC9:
                case 0xCA: case 0xCB: case 0xCC: case 0xCE: case 0xCF:
                case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4:
                case 0xD5: case 0xD6: case 0xD7: case 0xD8:
                    text[i] = '+'; break;
                case 0xDB:                     text[i] = '#'; break;
                default:                       text[i] = ' '; break;
            }
        }

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00 || c == ' ') {
                continue;
            }

            bool fontSmall = false;
            unsigned char styleByte = (i < styleBytes.size()) ? styleBytes[i] : 0x00;
            fontSmall = (styleByte & 0xF0) == 0x00;

            unsigned char colorIdx = styleByte & 0x0F;

            product->writeLineToPage(page, lineNum, i, std::string(1, c), colorIdx, fontSmall);
        }
    }
}

void CL650FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

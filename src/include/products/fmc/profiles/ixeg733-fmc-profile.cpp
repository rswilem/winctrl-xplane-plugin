#include "ixeg733-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <XPLMDataAccess.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

IXEG733FMCProfile::IXEG733FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<float>("ixeg/733/rheostats/light_fmc_pt_act", [product](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on") ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("ixeg/733/rheostats/light_fmc_pt_act");
    });
}

IXEG733FMCProfile::~IXEG733FMCProfile() {
    Dataref::getInstance()->unbind("ixeg/733/rheostats/light_fmc_pt_act");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
}

bool IXEG733FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("ixeg/733/FMC/cdu1_menu");
}

const std::vector<std::string> &IXEG733FMCProfile::displayDatarefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cdu1" : "cdu2";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        "ixeg/733/FMC/" + cdu + "D_pg_number",
                        "ixeg/733/FMC/" + cdu + "D_title",
                        "ixeg/733/FMC/" + cdu + "D_line1L_d",
                        "ixeg/733/FMC/" + cdu + "D_line1L_t",
                        "ixeg/733/FMC/" + cdu + "D_line1R_d",
                        "ixeg/733/FMC/" + cdu + "D_line1R_t",
                        "ixeg/733/FMC/" + cdu + "D_line2L_d",
                        "ixeg/733/FMC/" + cdu + "D_line2L_t",
                        "ixeg/733/FMC/" + cdu + "D_line2R_d",
                        "ixeg/733/FMC/" + cdu + "D_line2R_t",
                        "ixeg/733/FMC/" + cdu + "D_line3L_d",
                        "ixeg/733/FMC/" + cdu + "D_line3L_t",
                        "ixeg/733/FMC/" + cdu + "D_line3R_d",
                        "ixeg/733/FMC/" + cdu + "D_line3R_t",
                        "ixeg/733/FMC/" + cdu + "D_line4L_d",
                        "ixeg/733/FMC/" + cdu + "D_line4L_t",
                        "ixeg/733/FMC/" + cdu + "D_line4R_d",
                        "ixeg/733/FMC/" + cdu + "D_line4R_t",
                        "ixeg/733/FMC/" + cdu + "D_line5L_d",
                        "ixeg/733/FMC/" + cdu + "D_line5L_t",
                        "ixeg/733/FMC/" + cdu + "D_line5R_d",
                        "ixeg/733/FMC/" + cdu + "D_line5R_t",
                        "ixeg/733/FMC/" + cdu + "D_line6L_d",
                        "ixeg/733/FMC/" + cdu + "D_line6L_t",
                        "ixeg/733/FMC/" + cdu + "D_line6R_d",
                        "ixeg/733/FMC/" + cdu + "D_line6R_t",
                        "ixeg/733/FMC/" + cdu + "D_scrpad"})
        .first->second;
}

const std::vector<FMCButtonDef> &IXEG733FMCProfile::buttonDefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cdu1" : "cdu2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "ixeg/733/FMC/" + cdu + "_lsk_1L"},
                        {FMCKey::LSK2L, "ixeg/733/FMC/" + cdu + "_lsk_2L"},
                        {FMCKey::LSK3L, "ixeg/733/FMC/" + cdu + "_lsk_3L"},
                        {FMCKey::LSK4L, "ixeg/733/FMC/" + cdu + "_lsk_4L"},
                        {FMCKey::LSK5L, "ixeg/733/FMC/" + cdu + "_lsk_5L"},
                        {FMCKey::LSK6L, "ixeg/733/FMC/" + cdu + "_lsk_6L"},
                        {FMCKey::LSK1R, "ixeg/733/FMC/" + cdu + "_lsk_1R"},
                        {FMCKey::LSK2R, "ixeg/733/FMC/" + cdu + "_lsk_2R"},
                        {FMCKey::LSK3R, "ixeg/733/FMC/" + cdu + "_lsk_3R"},
                        {FMCKey::LSK4R, "ixeg/733/FMC/" + cdu + "_lsk_4R"},
                        {FMCKey::LSK5R, "ixeg/733/FMC/" + cdu + "_lsk_5R"},
                        {FMCKey::LSK6R, "ixeg/733/FMC/" + cdu + "_lsk_6R"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "ixeg/733/FMC/" + cdu + "_initref"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "ixeg/733/FMC/" + cdu + "_rte"},
                        {FMCKey::PFP3_CLB, "ixeg/733/FMC/" + cdu + "_clb"},
                        {FMCKey::PFP3_CRZ, "ixeg/733/FMC/" + cdu + "_crz"},
                        {FMCKey::PFP3_DES, "ixeg/733/FMC/" + cdu + "_des"},
                        {FMCKey::BRIGHTNESS_DOWN, "ixeg/733/rheostats/light_fmc_pt_act", -0.1},
                        {FMCKey::BRIGHTNESS_UP, "ixeg/733/rheostats/light_fmc_pt_act", 0.1},
                        {FMCKey::MENU, "ixeg/733/FMC/" + cdu + "_menu"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "ixeg/733/FMC/" + cdu + "_legs"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "ixeg/733/FMC/" + cdu + "_deparr"},
                        {FMCKey::PFP_HOLD, "ixeg/733/FMC/" + cdu + "_hold"},
                        {FMCKey::PROG, "ixeg/733/FMC/" + cdu + "_prog"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "ixeg/733/FMC/" + cdu + "_exec"},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_PERF}, "ixeg/733/FMC/" + cdu + "_n1limit"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "ixeg/733/FMC/" + cdu + "_fix"},
                        {FMCKey::PAGE_PREV, "ixeg/733/FMC/" + cdu + "_prev"},
                        {FMCKey::PAGE_NEXT, "ixeg/733/FMC/" + cdu + "_next"},
                        {FMCKey::KEY1, "ixeg/733/FMC/" + cdu + "_1"},
                        {FMCKey::KEY2, "ixeg/733/FMC/" + cdu + "_2"},
                        {FMCKey::KEY3, "ixeg/733/FMC/" + cdu + "_3"},
                        {FMCKey::KEY4, "ixeg/733/FMC/" + cdu + "_4"},
                        {FMCKey::KEY5, "ixeg/733/FMC/" + cdu + "_5"},
                        {FMCKey::KEY6, "ixeg/733/FMC/" + cdu + "_6"},
                        {FMCKey::KEY7, "ixeg/733/FMC/" + cdu + "_7"},
                        {FMCKey::KEY8, "ixeg/733/FMC/" + cdu + "_8"},
                        {FMCKey::KEY9, "ixeg/733/FMC/" + cdu + "_9"},
                        {FMCKey::PERIOD, "ixeg/733/FMC/" + cdu + "_dot"},
                        {FMCKey::KEY0, "ixeg/733/FMC/" + cdu + "_0"},
                        {FMCKey::PLUSMINUS, "ixeg/733/FMC/" + cdu + "_plus"},
                        {FMCKey::KEYA, "ixeg/733/FMC/" + cdu + "_A"},
                        {FMCKey::KEYB, "ixeg/733/FMC/" + cdu + "_B"},
                        {FMCKey::KEYC, "ixeg/733/FMC/" + cdu + "_C"},
                        {FMCKey::KEYD, "ixeg/733/FMC/" + cdu + "_D"},
                        {FMCKey::KEYE, "ixeg/733/FMC/" + cdu + "_E"},
                        {FMCKey::KEYF, "ixeg/733/FMC/" + cdu + "_F"},
                        {FMCKey::KEYG, "ixeg/733/FMC/" + cdu + "_G"},
                        {FMCKey::KEYH, "ixeg/733/FMC/" + cdu + "_H"},
                        {FMCKey::KEYI, "ixeg/733/FMC/" + cdu + "_I"},
                        {FMCKey::KEYJ, "ixeg/733/FMC/" + cdu + "_J"},
                        {FMCKey::KEYK, "ixeg/733/FMC/" + cdu + "_K"},
                        {FMCKey::KEYL, "ixeg/733/FMC/" + cdu + "_L"},
                        {FMCKey::KEYM, "ixeg/733/FMC/" + cdu + "_M"},
                        {FMCKey::KEYN, "ixeg/733/FMC/" + cdu + "_N"},
                        {FMCKey::KEYO, "ixeg/733/FMC/" + cdu + "_O"},
                        {FMCKey::KEYP, "ixeg/733/FMC/" + cdu + "_P"},
                        {FMCKey::KEYQ, "ixeg/733/FMC/" + cdu + "_Q"},
                        {FMCKey::KEYR, "ixeg/733/FMC/" + cdu + "_R"},
                        {FMCKey::KEYS, "ixeg/733/FMC/" + cdu + "_S"},
                        {FMCKey::KEYT, "ixeg/733/FMC/" + cdu + "_T"},
                        {FMCKey::KEYU, "ixeg/733/FMC/" + cdu + "_U"},
                        {FMCKey::KEYV, "ixeg/733/FMC/" + cdu + "_V"},
                        {FMCKey::KEYW, "ixeg/733/FMC/" + cdu + "_W"},
                        {FMCKey::KEYX, "ixeg/733/FMC/" + cdu + "_X"},
                        {FMCKey::KEYY, "ixeg/733/FMC/" + cdu + "_Y"},
                        {FMCKey::KEYZ, "ixeg/733/FMC/" + cdu + "_Z"},
                        {FMCKey::SPACE, "ixeg/733/FMC/" + cdu + "_sp"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "ixeg/733/FMC/" + cdu + "_del"},
                        {FMCKey::SLASH, "ixeg/733/FMC/" + cdu + "_slash"},
                        {FMCKey::CLR, "ixeg/733/FMC/" + cdu + "_clr"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &IXEG733FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &IXEG733FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'W', FMCTextColor::COLOR_WHITE},
        {'G', FMCTextColor::COLOR_GREEN}, // Green text
        {'S', FMCTextColor::COLOR_GREEN}, // Green text (Small)
        {'I', FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_GREEN)},
    };
    return colMap;
}

void IXEG733FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

std::pair<std::string, std::vector<char>> IXEG733FMCProfile::processIxegText(const std::vector<unsigned char> &characters) {
    std::string text;
    std::vector<char> colors;

    bool inInvertedMode = false;
    bool inSmallMode = false;

    for (size_t i = 0; i < characters.size(); ++i) {
        unsigned char c = characters[i];

        if (c == 0x00) {
            break;
        }

        if (c == 0xA3) {
            // 0xA3 = start marker for white text until next space
            inSmallMode = true;
            continue;
        }

        if (c >= 0x20 && c <= 0x7E) {
            char ch = static_cast<char>(c);

            // Check for $$ at current position
            if (!inInvertedMode && ch == '$' && i + 1 < characters.size() &&
                characters[i + 1] >= 0x20 && characters[i + 1] <= 0x7E &&
                static_cast<char>(characters[i + 1]) == '$') {
                inInvertedMode = true;
                i++;      // Skip the second $
                continue; // Don't add $$ to the output text
            }

            // Check if we hit a space while in inverted or white mode
            if (ch == ' ') {
                if (inInvertedMode) {
                    inInvertedMode = false;
                }
                if (inSmallMode) {
                    inSmallMode = false;
                }
            }

            text += ch;
            colors.push_back(inInvertedMode ? 'I' : (inSmallMode ? 'S' : 'G'));
        }
    }

    return std::make_pair(text, colors);
}

void IXEG733FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (const auto &ref : displayDatarefs()) {
        std::vector<unsigned char> characters = datarefManager->getCached<std::vector<unsigned char>>(ref.c_str());
        if (characters.empty()) {
            continue;
        }

        auto [text, colors] = processIxegText(characters);
        if (ref.ends_with("D_title")) {
            for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
                char c = text[i];
                char color = i < colors.size() ? colors[i] : 'G';
                product->writeLineToPage(page, 0, i, std::string(1, c), color, color == 'S');
            }
            continue;
        }

        if (ref.ends_with("D_pg_number")) {
            int startPos = ProductFMC::PageCharsPerLine - (int) text.length();
            if (startPos > 0) {
                for (int i = 0; i < text.size() && (startPos + i) < ProductFMC::PageCharsPerLine; ++i) {
                    char c = text[i];
                    char color = i < colors.size() ? colors[i] : 'G';
                    product->writeLineToPage(page, 0, startPos + i, std::string(1, c), color, color == 'S');
                }
            }
            continue;
        }

        if (ref.ends_with("D_scrpad")) {
            for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
                char c = text[i];
                char color = i < colors.size() ? colors[i] : 'G';
                product->writeLineToPage(page, 13, i, std::string(1, c), color, color == 'S');
            }
            continue;
        }

        if (ref.find("D_line") == 0) {
            size_t linePos = ref.find("line") + 4;
            if (linePos + 2 < ref.length()) {
                char lineChar = ref[linePos];
                char sideChar = ref[linePos + 1];
                char typeChar = ref[ref.length() - 1];

                if (lineChar >= '1' && lineChar <= '6' &&
                    (sideChar == 'L' || sideChar == 'R') &&
                    (typeChar == 'd' || typeChar == 't')) {
                    int lineNum = lineChar - '0';
                    bool isLeftSide = (sideChar == 'L');
                    bool isTitle = (typeChar == 't');

                    int displayLine = (lineNum * 2) - (isTitle ? 1 : 0);

                    int startPos = 0;
                    if (!isLeftSide) {
                        startPos = ProductFMC::PageCharsPerLine - (int) text.length();
                        if (startPos < ProductFMC::PageCharsPerLine / 2) {
                            startPos = ProductFMC::PageCharsPerLine / 2;
                        }
                    }

                    for (int i = 0; i < text.size() && (startPos + i) < ProductFMC::PageCharsPerLine; ++i) {
                        char c = text[i];
                        char color = i < colors.size() ? colors[i] : 'G';
                        product->writeLineToPage(page, displayLine, startPos + i, std::string(1, c), color, isTitle || color == 'S');
                    }
                }
            }
            continue;
        }
    }
}

void IXEG733FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (phase == xplm_CommandContinue) {
        return;
    }

    if (std::holds_alternative<FMCKey>(button->key) &&
        std::get<FMCKey>(button->key) == FMCKey::CLR) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    } else {
        if (std::fabs(button->value) > std::numeric_limits<double>::epsilon()) {
            if (phase != xplm_CommandBegin) {
                return;
            }

            float currentValue = Dataref::getInstance()->get<float>(button->dataref.c_str());
            Dataref::getInstance()->set<float>(button->dataref.c_str(), std::clamp(currentValue + button->value, 0.0, 1.0));
        } else {
            Dataref::getInstance()->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? 1 : 0);
        }
    }
}

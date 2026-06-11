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
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("ixeg/733/rheostats/light_fmc_pt_act");
    }, this);
}

IXEG733FMCProfile::~IXEG733FMCProfile() {
    Dataref::getInstance()->unbindAll(this);
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
                        {FMCKey::LSK1L, "ixeg/733/FMC/" + cdu + "_lsk_1L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2L, "ixeg/733/FMC/" + cdu + "_lsk_2L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3L, "ixeg/733/FMC/" + cdu + "_lsk_3L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4L, "ixeg/733/FMC/" + cdu + "_lsk_4L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5L, "ixeg/733/FMC/" + cdu + "_lsk_5L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6L, "ixeg/733/FMC/" + cdu + "_lsk_6L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK1R, "ixeg/733/FMC/" + cdu + "_lsk_1R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2R, "ixeg/733/FMC/" + cdu + "_lsk_2R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3R, "ixeg/733/FMC/" + cdu + "_lsk_3R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4R, "ixeg/733/FMC/" + cdu + "_lsk_4R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5R, "ixeg/733/FMC/" + cdu + "_lsk_5R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6R, "ixeg/733/FMC/" + cdu + "_lsk_6R", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "ixeg/733/FMC/" + cdu + "_initref", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "ixeg/733/FMC/" + cdu + "_rte", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_CLB, "ixeg/733/FMC/" + cdu + "_clb", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_CRZ, "ixeg/733/FMC/" + cdu + "_crz", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_DES, "ixeg/733/FMC/" + cdu + "_des", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::BRIGHTNESS_DOWN, "ixeg/733/rheostats/light_fmc_pt_act", FMCDatarefType::ADJUST_VALUE, -0.1},
                        {FMCKey::BRIGHTNESS_UP, "ixeg/733/rheostats/light_fmc_pt_act", FMCDatarefType::ADJUST_VALUE, 0.1},
                        {FMCKey::MENU, "ixeg/733/FMC/" + cdu + "_menu", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "ixeg/733/FMC/" + cdu + "_legs", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "ixeg/733/FMC/" + cdu + "_deparr", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP_HOLD, "ixeg/733/FMC/" + cdu + "_hold", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PROG, "ixeg/733/FMC/" + cdu + "_prog", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "ixeg/733/FMC/" + cdu + "_exec", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_PERF}, "ixeg/733/FMC/" + cdu + "_n1limit", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "ixeg/733/FMC/" + cdu + "_fix", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_PREV, "ixeg/733/FMC/" + cdu + "_prev", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_NEXT, "ixeg/733/FMC/" + cdu + "_next", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY1, "ixeg/733/FMC/" + cdu + "_1", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY2, "ixeg/733/FMC/" + cdu + "_2", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY3, "ixeg/733/FMC/" + cdu + "_3", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY4, "ixeg/733/FMC/" + cdu + "_4", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY5, "ixeg/733/FMC/" + cdu + "_5", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY6, "ixeg/733/FMC/" + cdu + "_6", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY7, "ixeg/733/FMC/" + cdu + "_7", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY8, "ixeg/733/FMC/" + cdu + "_8", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY9, "ixeg/733/FMC/" + cdu + "_9", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PERIOD, "ixeg/733/FMC/" + cdu + "_dot", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY0, "ixeg/733/FMC/" + cdu + "_0", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PLUSMINUS, "ixeg/733/FMC/" + cdu + "_plus", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYA, "ixeg/733/FMC/" + cdu + "_A", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYB, "ixeg/733/FMC/" + cdu + "_B", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYC, "ixeg/733/FMC/" + cdu + "_C", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYD, "ixeg/733/FMC/" + cdu + "_D", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYE, "ixeg/733/FMC/" + cdu + "_E", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYF, "ixeg/733/FMC/" + cdu + "_F", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYG, "ixeg/733/FMC/" + cdu + "_G", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYH, "ixeg/733/FMC/" + cdu + "_H", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYI, "ixeg/733/FMC/" + cdu + "_I", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYJ, "ixeg/733/FMC/" + cdu + "_J", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYK, "ixeg/733/FMC/" + cdu + "_K", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYL, "ixeg/733/FMC/" + cdu + "_L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYM, "ixeg/733/FMC/" + cdu + "_M", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYN, "ixeg/733/FMC/" + cdu + "_N", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYO, "ixeg/733/FMC/" + cdu + "_O", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYP, "ixeg/733/FMC/" + cdu + "_P", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYQ, "ixeg/733/FMC/" + cdu + "_Q", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYR, "ixeg/733/FMC/" + cdu + "_R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYS, "ixeg/733/FMC/" + cdu + "_S", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYT, "ixeg/733/FMC/" + cdu + "_T", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYU, "ixeg/733/FMC/" + cdu + "_U", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYV, "ixeg/733/FMC/" + cdu + "_V", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYW, "ixeg/733/FMC/" + cdu + "_W", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYX, "ixeg/733/FMC/" + cdu + "_X", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYY, "ixeg/733/FMC/" + cdu + "_Y", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYZ, "ixeg/733/FMC/" + cdu + "_Z", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SPACE, "ixeg/733/FMC/" + cdu + "_sp", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "ixeg/733/FMC/" + cdu + "_del", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SLASH, "ixeg/733/FMC/" + cdu + "_slash", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::CLR, "ixeg/733/FMC/" + cdu + "_clr", FMCDatarefType::EXECUTE_CMD_PHASED}})
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

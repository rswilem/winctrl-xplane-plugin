#include "magknight787-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

Magknight787FMCProfile::Magknight787FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    const char *execLight = isFirstOfficer() ? "sim/cockpit2/radios/indicators/fms_exec_light_copilot" : "sim/cockpit2/radios/indicators/fms_exec_light_pilot";
    Dataref::getInstance()->monitorExistingDataref<bool>(execLight, [product](bool lit) {
        product->setLedBrightness(FMCLed::PFP_EXEC, lit ? 1 : 0);
    },
        this);

    auto updateBrightness = [product]() {
        auto datarefManager = Dataref::getInstance();
        std::vector<float> brightness = datarefManager->getCached<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
        bool powered = datarefManager->getCached<float>("aero787/electrical/totalWatts") > 2000.0f;
        if (brightness.size() <= 11) {
            return;
        }

        product->setLedBrightness(FMCLed::BACKLIGHT, powered ? static_cast<uint8_t>(std::clamp(brightness[8], 0.0f, 1.0f) * 255) : 0);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, powered ? static_cast<uint8_t>(std::clamp(brightness[11], 0.0f, 1.0f) * 255) : 0);
    };

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [updateBrightness](const std::vector<float> &) {
        updateBrightness();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("aero787/electrical/totalWatts", [updateBrightness](float) {
        updateBrightness();
    },
        this);

    updateBrightness();
}

bool Magknight787FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aero787/cduL/ExecD");
}

bool Magknight787FMCProfile::isFirstOfficer() const {
    return product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER;
}

const std::vector<std::string> &Magknight787FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        const std::string prefix = std::string("sim/cockpit2/radios/indicators/fms_cdu") + (isFirstOfficer() ? "2" : "1");
        std::vector<std::string> datarefs;
        for (int line = 0; line < static_cast<int>(ProductFMC::PageLines); ++line) {
            datarefs.push_back(prefix + "_text_line" + std::to_string(line));
            datarefs.push_back(prefix + "_style_line" + std::to_string(line));
        }
        it = cache.emplace(product->deviceVariant, std::move(datarefs)).first;
    }
    return it->second;
}

const std::vector<FMCButtonDef> &Magknight787FMCProfile::buttonDefs() const {
    const std::string cdu = isFirstOfficer() ? "aero787/cduR/" : "aero787/cduL/";
    const std::string fms = isFirstOfficer() ? "sim/FMS2/" : "sim/FMS/";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, fms + "ls_1l"},
                        {FMCKey::LSK2L, fms + "ls_2l"},
                        {FMCKey::LSK3L, fms + "ls_3l"},
                        {FMCKey::LSK4L, fms + "ls_4l"},
                        {FMCKey::LSK5L, fms + "ls_5l"},
                        {FMCKey::LSK6L, fms + "ls_6l"},
                        {FMCKey::LSK1R, fms + "ls_1r"},
                        {FMCKey::LSK2R, fms + "ls_2r"},
                        {FMCKey::LSK3R, fms + "ls_3r"},
                        {FMCKey::LSK4R, fms + "ls_4r"},
                        {FMCKey::LSK5R, fms + "ls_5r"},
                        {FMCKey::LSK6R, fms + "ls_6r"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, fms + "index"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, fms + "fpln"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, fms + "dep_arr"},
                        {std::vector<FMCKey>{FMCKey::PFP7_VNAV, FMCKey::PFP4_VNAV, FMCKey::PFP3_CLB, FMCKey::MCDU_PERF}, fms + "clb"},
                        {FMCKey::PFP3_CRZ, fms + "crz"},
                        {FMCKey::PFP3_DES, fms + "des"},
                        {FMCKey::PFP_FIX, fms + "fix"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN}, fms + "legs"},
                        {FMCKey::PFP_HOLD, fms + "hold"},
                        {FMCKey::PROG, fms + "prog"},
                        {FMCKey::MENU, fms + "menu"},
                        {std::vector<FMCKey>{FMCKey::PFP7_NAV_RAD, FMCKey::PFP4_NAV_RAD, FMCKey::MCDU_RAD_NAV}, fms + "navrad"},
                        {std::vector<FMCKey>{FMCKey::PFP4_ATC, FMCKey::MCDU_ATC_COMM}, fms + "atc_comm"},
                        {std::vector<FMCKey>{FMCKey::PFP7_ALTN, FMCKey::MCDU_DIR}, fms + "dir_intc"},
                        {FMCKey::MCDU_FUEL_PRED, fms + "fuel_pred"},
                        {FMCKey::MCDU_PAGE_UP, fms + "up"},
                        {FMCKey::MCDU_PAGE_DOWN, fms + "down"},
                        {std::vector<FMCKey>{FMCKey::PFP7_FMC_COMM, FMCKey::PFP4_FMC_COMM, FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_DATA, FMCKey::MCDU_EMPTY_BOTTOM_LEFT, FMCKey::BRIGHTNESS_DOWN, FMCKey::BRIGHTNESS_UP}, ""},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, cdu + "Exec"},
                        {FMCKey::PAGE_PREV, cdu + "Prev"},
                        {FMCKey::PAGE_NEXT, cdu + "Next"},
                        {FMCKey::KEY0, cdu + "key_0"},
                        {FMCKey::KEY1, cdu + "key_1"},
                        {FMCKey::KEY2, cdu + "key_2"},
                        {FMCKey::KEY3, cdu + "key_3"},
                        {FMCKey::KEY4, cdu + "key_4"},
                        {FMCKey::KEY5, cdu + "key_5"},
                        {FMCKey::KEY6, cdu + "key_6"},
                        {FMCKey::KEY7, cdu + "key_7"},
                        {FMCKey::KEY8, cdu + "key_8"},
                        {FMCKey::KEY9, cdu + "key_9"},
                        {FMCKey::KEYA, cdu + "key_A"},
                        {FMCKey::KEYB, cdu + "key_B"},
                        {FMCKey::KEYC, cdu + "key_C"},
                        {FMCKey::KEYD, cdu + "key_D"},
                        {FMCKey::KEYE, cdu + "key_E"},
                        {FMCKey::KEYF, cdu + "key_F"},
                        {FMCKey::KEYG, cdu + "key_G"},
                        {FMCKey::KEYH, cdu + "key_H"},
                        {FMCKey::KEYI, cdu + "key_I"},
                        {FMCKey::KEYJ, cdu + "key_J"},
                        {FMCKey::KEYK, cdu + "key_K"},
                        {FMCKey::KEYL, cdu + "key_L"},
                        {FMCKey::KEYM, cdu + "key_M"},
                        {FMCKey::KEYN, cdu + "key_N"},
                        {FMCKey::KEYO, cdu + "key_O"},
                        {FMCKey::KEYP, cdu + "key_P"},
                        {FMCKey::KEYQ, cdu + "key_Q"},
                        {FMCKey::KEYR, cdu + "key_R"},
                        {FMCKey::KEYS, cdu + "key_S"},
                        {FMCKey::KEYT, cdu + "key_T"},
                        {FMCKey::KEYU, cdu + "key_U"},
                        {FMCKey::KEYV, cdu + "key_V"},
                        {FMCKey::KEYW, cdu + "key_W"},
                        {FMCKey::KEYX, cdu + "key_X"},
                        {FMCKey::KEYY, cdu + "key_Y"},
                        {FMCKey::KEYZ, cdu + "key_Z"},
                        {FMCKey::PERIOD, cdu + "Dot"},
                        {FMCKey::PLUSMINUS, cdu + "Minus"},
                        {FMCKey::SLASH, cdu + "Slash"},
                        {FMCKey::SPACE, cdu + "Space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, cdu + "Delete"},
                        {FMCKey::CLR, cdu + "Clear"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &Magknight787FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &Magknight787FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
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
        {0x44, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_GREEN)},
        {0x45, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_MAGENTA)},
        {0x46, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_AMBER)},
        {0x47, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},
    };
    return colMap;
}

void Magknight787FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case 0x1C:
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case 0x1D:
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case 24:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 25:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case 26:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 27:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        default:
            if (character < 32 || character > 126) {
                buffer->push_back(' ');
            } else {
                buffer->push_back(character);
            }
            break;
    }
}

void Magknight787FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    const std::string prefix = std::string("sim/cockpit2/radios/indicators/fms_cdu") + (isFirstOfficer() ? "2" : "1");
    const std::vector<std::pair<std::string, unsigned char>> symbols = {
        {"°", 0x1C},
        {"☐", 0x1D},
        {"□", 0x1D},
        {"↑", 24},
        {"↓", 25},
        {"→", 26},
        {"←", 27},
        {"▶", '>'},
        {"◀", '<'}};

    auto datarefManager = Dataref::getInstance();
    for (int lineNum = 0; lineNum < static_cast<int>(ProductFMC::PageLines); ++lineNum) {
        std::string text = datarefManager->getCached<std::string>((prefix + "_text_line" + std::to_string(lineNum)).c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>((prefix + "_style_line" + std::to_string(lineNum)).c_str());

        for (const auto &symbol : symbols) {
            size_t pos = 0;
            while ((pos = text.find(symbol.first, pos)) != std::string::npos) {
                text.replace(pos, symbol.first.length(), std::string(1, static_cast<char>(symbol.second)));
                pos += 1;
            }
        }

        for (size_t i = 0; i < text.size(); ++i) {
            if (static_cast<unsigned char>(text[i]) > 127) {
                text[i] = '?';
            }
        }

        for (int i = 0; i < static_cast<int>(text.size()) && i < static_cast<int>(ProductFMC::PageCharsPerLine); ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            unsigned char styleByte = i < static_cast<int>(styleBytes.size()) ? styleBytes[i] : 0x00;
            bool fontSmall = (styleByte & 0x80) == 0x00;

            product->writeLineToPage(page, lineNum, i, std::string(1, c), styleByte & 0x47, fontSmall);
        }
    }
}

void Magknight787FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

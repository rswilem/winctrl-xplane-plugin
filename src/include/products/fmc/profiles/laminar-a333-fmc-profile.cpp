#include "laminar-a333-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

LaminarA333FMCProfile::LaminarA333FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontAirbus);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 6) {
            return;
        }

        uint8_t target = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on") ? brightness[6] * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    }, this);

    product->setLedBrightness(FMCLed::BACKLIGHT, 128);
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 128);
}

bool LaminarA333FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/A333/ckpt_temp");
}

const std::vector<std::string> &LaminarA333FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<std::string>{
                        // Text content for lines 0-15
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line0",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line1",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line2",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line3",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line4",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line5",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line6",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line7",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line8",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line9",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line10",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line11",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line12",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line13",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line14",
                        "sim/cockpit2/radios/indicators/fms_cdu1_text_line15"})
        .first->second;
}

const std::vector<FMCButtonDef> &LaminarA333FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "sim/FMS/ls_1l"},
                        {FMCKey::LSK2L, "sim/FMS/ls_2l"},
                        {FMCKey::LSK3L, "sim/FMS/ls_3l"},
                        {FMCKey::LSK4L, "sim/FMS/ls_4l"},
                        {FMCKey::LSK5L, "sim/FMS/ls_5l"},
                        {FMCKey::LSK6L, "sim/FMS/ls_6l"},
                        {FMCKey::LSK1R, "sim/FMS/ls_1r"},
                        {FMCKey::LSK2R, "sim/FMS/ls_2r"},
                        {FMCKey::LSK3R, "sim/FMS/ls_3r"},
                        {FMCKey::LSK4R, "sim/FMS/ls_4r"},
                        {FMCKey::LSK5R, "sim/FMS/ls_5r"},
                        {FMCKey::LSK6R, "sim/FMS/ls_6r"},
                        {FMCKey::MCDU_DIR, "sim/FMS/dir_intc"},
                        {FMCKey::PROG, "sim/FMS/prog"},
                        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, "sim/FMS/perf"},
                        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP_INIT_REF}, "sim/FMS/index"},
                        {FMCKey::MCDU_DATA, "sim/FMS/data"},
                        {FMCKey::MCDU_EMPTY_TOP_RIGHT, ""},
                        {FMCKey::BRIGHTNESS_UP, "laminar/A333/buttons/fms1_brightness_up"},
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS}, "sim/FMS/fpln"},
                        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "sim/FMS/navrad"},
                        {FMCKey::MCDU_FUEL_PRED, "sim/FMS/fuel_pred"},
                        {FMCKey::MCDU_SEC_FPLN, "sim/FMS/sec_fpln"},
                        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, "sim/FMS/atc_comm"},
                        {FMCKey::MENU, "sim/FMS/menu"},
                        {FMCKey::BRIGHTNESS_DOWN, "laminar/A333/buttons/fms1_brightness_dn"},
                        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, "sim/FMS/airport"},
                        {FMCKey::MCDU_EMPTY_BOTTOM_LEFT, ""},
                        {FMCKey::PAGE_PREV, "sim/FMS/prev"},
                        {FMCKey::MCDU_PAGE_UP, "sim/FMS/up"},
                        {FMCKey::PAGE_NEXT, "sim/FMS/next"},
                        {FMCKey::MCDU_PAGE_DOWN, "sim/FMS/down"},
                        {FMCKey::KEY1, "sim/FMS/key_1"},
                        {FMCKey::KEY2, "sim/FMS/key_2"},
                        {FMCKey::KEY3, "sim/FMS/key_3"},
                        {FMCKey::KEY4, "sim/FMS/key_4"},
                        {FMCKey::KEY5, "sim/FMS/key_5"},
                        {FMCKey::KEY6, "sim/FMS/key_6"},
                        {FMCKey::KEY7, "sim/FMS/key_7"},
                        {FMCKey::KEY8, "sim/FMS/key_8"},
                        {FMCKey::KEY9, "sim/FMS/key_9"},
                        {FMCKey::PERIOD, "sim/FMS/key_period"},
                        {FMCKey::KEY0, "sim/FMS/key_0"},
                        {FMCKey::PLUSMINUS, "sim/FMS/key_minus"},
                        {FMCKey::KEYA, "sim/FMS/key_A"},
                        {FMCKey::KEYB, "sim/FMS/key_B"},
                        {FMCKey::KEYC, "sim/FMS/key_C"},
                        {FMCKey::KEYD, "sim/FMS/key_D"},
                        {FMCKey::KEYE, "sim/FMS/key_E"},
                        {FMCKey::KEYF, "sim/FMS/key_F"},
                        {FMCKey::KEYG, "sim/FMS/key_G"},
                        {FMCKey::KEYH, "sim/FMS/key_H"},
                        {FMCKey::KEYI, "sim/FMS/key_I"},
                        {FMCKey::KEYJ, "sim/FMS/key_J"},
                        {FMCKey::KEYK, "sim/FMS/key_K"},
                        {FMCKey::KEYL, "sim/FMS/key_L"},
                        {FMCKey::KEYM, "sim/FMS/key_M"},
                        {FMCKey::KEYN, "sim/FMS/key_N"},
                        {FMCKey::KEYO, "sim/FMS/key_O"},
                        {FMCKey::KEYP, "sim/FMS/key_P"},
                        {FMCKey::KEYQ, "sim/FMS/key_Q"},
                        {FMCKey::KEYR, "sim/FMS/key_R"},
                        {FMCKey::KEYS, "sim/FMS/key_S"},
                        {FMCKey::KEYT, "sim/FMS/key_T"},
                        {FMCKey::KEYU, "sim/FMS/key_U"},
                        {FMCKey::KEYV, "sim/FMS/key_V"},
                        {FMCKey::KEYW, "sim/FMS/key_W"},
                        {FMCKey::KEYX, "sim/FMS/key_X"},
                        {FMCKey::KEYY, "sim/FMS/key_Y"},
                        {FMCKey::KEYZ, "sim/FMS/key_Z"},
                        {FMCKey::SLASH, "sim/FMS/key_slash"},
                        {FMCKey::SPACE, "sim/FMS/key_space"},
                        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_DEL}, "sim/FMS/key_overfly"},
                        {FMCKey::CLR, "sim/FMS/key_clear"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &LaminarA333FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &LaminarA333FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x06, FMCTextColor::COLOR_AMBER},
    };
    return colMap;
}

void LaminarA333FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '<':
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case '>':
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case 30: // Up arrow
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            }
            break;

        case 31: // Down arrow
            if (isFontSmall) {
                buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            } else {
                buffer->push_back(character);
            }
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void LaminarA333FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (int lineNum = 0; lineNum < std::min(ProductFMC::PageLines, (unsigned int) 16); ++lineNum) {
        std::string textDataref = "sim/cockpit2/radios/indicators/fms_cdu1_text_line" + std::to_string(lineNum);
        std::string styleDataref = "sim/cockpit2/radios/indicators/fms_cdu1_style_line" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        // Replace all special characters with placeholders
        const std::vector<std::pair<std::string, unsigned char>> symbols = {
            {"\u2190", '<'},
            {"\u2192", '>'},
            {"\u2191", 30},
            {"\u2193", 31},
            {"\u2610", '#'},
            {"\u00B0", '`'}};

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

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }

            bool fontSmall = false;
            unsigned char styleByte = (i < styleBytes.size()) ? styleBytes[i] : 0x00;
            fontSmall = (styleByte & 0xF0) == 0x00;

            int displayLine = lineNum;
            if (displayLine >= ProductFMC::PageLines) {
                break;
            }

            product->writeLineToPage(page, displayLine, i, std::string(1, c), styleByte & 0x0F, fontSmall);
        }
    }
}

void LaminarA333FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

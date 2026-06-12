#include "jar330-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

JAR330FMCProfile::JAR330FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontAirbus);

    // JAR A330 MCDU brightness control
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

bool JAR330FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("jd/mcdu/big/line00col0");
}

const std::vector<std::string> &JAR330FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<std::string>{
                        // Standard X-Plane FMS datarefs: 14 lines of text + style
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
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line0",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line1",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line2",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line3",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line4",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line5",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line6",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line7",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line8",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line9",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line10",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line11",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line12",
                        "sim/cockpit2/radios/indicators/fms_cdu1_style_line13",
                    })
        .first->second;
}

const std::vector<FMCButtonDef> &JAR330FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(FMCDeviceVariant::VARIANT_CAPTAIN,
                    std::vector<FMCButtonDef>{
                        // Left side LSKs (6 keys)
                        {FMCKey::LSK1L, "jd/mcdu/click_l1"},
                        {FMCKey::LSK2L, "jd/mcdu/click_l2"},
                        {FMCKey::LSK3L, "jd/mcdu/click_l3"},
                        {FMCKey::LSK4L, "jd/mcdu/click_l4"},
                        {FMCKey::LSK5L, "jd/mcdu/click_l5"},
                        {FMCKey::LSK6L, "jd/mcdu/click_l6"},

                        // Right side LSKs (6 keys)
                        {FMCKey::LSK1R, "jd/mcdu/click_r1"},
                        {FMCKey::LSK2R, "jd/mcdu/click_r2"},
                        {FMCKey::LSK3R, "jd/mcdu/click_r3"},
                        {FMCKey::LSK4R, "jd/mcdu/click_r4"},
                        {FMCKey::LSK5R, "jd/mcdu/click_r5"},
                        {FMCKey::LSK6R, "jd/mcdu/click_r6"},

                        // Mode buttons
                        {FMCKey::MCDU_DIR, "jd/mcdu/click_dir"},
                        {FMCKey::PROG, "jd/mcdu/click_prog"},
                        {FMCKey::MCDU_PERF, "jd/mcdu/click_perf"},
                        {FMCKey::MCDU_INIT, "jd/mcdu/click_sec"},
                        {FMCKey::MCDU_DATA, "jd/mcdu/click_data"},
                        {FMCKey::BRIGHTNESS_UP, "jd/mcdu/click_br_up"},
                        {FMCKey::MCDU_FPLN, "jd/mcdu/click_fpln"},
                        {FMCKey::MCDU_RAD_NAV, "jd/mcdu/click_radnav"},
                        {FMCKey::MCDU_FUEL_PRED, "jd/mcdu/click_fuel"},
                        {FMCKey::MCDU_SEC_FPLN, "jd/mcdu/click_sec"},
                        {FMCKey::MCDU_ATC_COMM, "jd/mcdu/click_atc"},
                        {FMCKey::MENU, "jd/mcdu/click_mcdumenu"},
                        {FMCKey::BRIGHTNESS_DOWN, "jd/mcdu/click_br_dn"},
                        {FMCKey::MCDU_AIRPORT, "jd/mcdu/click_airp"},

                        // Navigation buttons
                        {FMCKey::PAGE_PREV, "jd/mcdu/click_left"},
                        {FMCKey::MCDU_PAGE_UP, "jd/mcdu/click_up"},
                        {FMCKey::PAGE_NEXT, "jd/mcdu/click_right"},
                        {FMCKey::MCDU_PAGE_DOWN, "jd/mcdu/click_dn"},

                        // Keypad number keys
                        {FMCKey::KEY1, "jd/mcdu/click_1"},
                        {FMCKey::KEY2, "jd/mcdu/click_2"},
                        {FMCKey::KEY3, "jd/mcdu/click_3"},
                        {FMCKey::KEY4, "jd/mcdu/click_4"},
                        {FMCKey::KEY5, "jd/mcdu/click_5"},
                        {FMCKey::KEY6, "jd/mcdu/click_6"},
                        {FMCKey::KEY7, "jd/mcdu/click_7"},
                        {FMCKey::KEY8, "jd/mcdu/click_8"},
                        {FMCKey::KEY9, "jd/mcdu/click_9"},
                        {FMCKey::PERIOD, "jd/mcdu/click_dot"},
                        {FMCKey::KEY0, "jd/mcdu/click_0"},
                        {FMCKey::PLUSMINUS, "jd/mcdu/click_plusmin"},

                        // Keypad letter keys
                        {FMCKey::KEYA, "jd/mcdu/click_a"},
                        {FMCKey::KEYB, "jd/mcdu/click_b"},
                        {FMCKey::KEYC, "jd/mcdu/click_c"},
                        {FMCKey::KEYD, "jd/mcdu/click_d"},
                        {FMCKey::KEYE, "jd/mcdu/click_e"},
                        {FMCKey::KEYF, "jd/mcdu/click_f"},
                        {FMCKey::KEYG, "jd/mcdu/click_g"},
                        {FMCKey::KEYH, "jd/mcdu/click_h"},
                        {FMCKey::KEYI, "jd/mcdu/click_i"},
                        {FMCKey::KEYJ, "jd/mcdu/click_j"},
                        {FMCKey::KEYK, "jd/mcdu/click_k"},
                        {FMCKey::KEYL, "jd/mcdu/click_l"},
                        {FMCKey::KEYM, "jd/mcdu/click_m"},
                        {FMCKey::KEYN, "jd/mcdu/click_n"},
                        {FMCKey::KEYO, "jd/mcdu/click_o"},
                        {FMCKey::KEYP, "jd/mcdu/click_p"},
                        {FMCKey::KEYQ, "jd/mcdu/click_q"},
                        {FMCKey::KEYR, "jd/mcdu/click_r"},
                        {FMCKey::KEYS, "jd/mcdu/click_s"},
                        {FMCKey::KEYT, "jd/mcdu/click_t"},
                        {FMCKey::KEYU, "jd/mcdu/click_u"},
                        {FMCKey::KEYV, "jd/mcdu/click_v"},
                        {FMCKey::KEYW, "jd/mcdu/click_w"},
                        {FMCKey::KEYX, "jd/mcdu/click_x"},
                        {FMCKey::KEYY, "jd/mcdu/click_y"},
                        {FMCKey::KEYZ, "jd/mcdu/click_z"},
                        {FMCKey::SLASH, "jd/mcdu/click_slash"},
                        {FMCKey::SPACE, "jd/mcdu/click_sp"},
                        {FMCKey::MCDU_OVERFLY, "jd/mcdu/click_ovfy"},
                        {FMCKey::CLR, "jd/mcdu/click_clr"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &JAR330FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &JAR330FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x06, FMCTextColor::COLOR_AMBER},
    };
    return colMap;
}

void JAR330FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
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

void JAR330FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();

    // Standard X-Plane FMS datarefs
    for (int lineNum = 0; lineNum < 14; ++lineNum) {
        std::string textDataref = "sim/cockpit2/radios/indicators/fms_cdu1_text_line" + std::to_string(lineNum);
        std::string styleDataref = "sim/cockpit2/radios/indicators/fms_cdu1_style_line" + std::to_string(lineNum);

        std::string text = datarefManager->getCached<std::string>(textDataref.c_str());
        if (text.empty()) {
            continue;
        }

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleDataref.c_str());

        // Replace all special characters with placeholders
        const std::vector<std::pair<std::string, unsigned char>> symbols = {
            {"←", '<'},
            {"→", '>'},
            {"↑", 30},
            {"↓", 31},
            {"☐", '#'},
            {"°", '`'}};

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

            product->writeLineToPage(page, lineNum, i, std::string(1, c), styleByte & 0x0F, fontSmall);
        }
    }
}

void JAR330FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // JAR MCDU buttons are SET_VALUE_PHASED type - write 1 on begin, 0 on end
    if (button->datarefType == FMCDatarefType::SET_VALUE_PHASED) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        datarefManager->set<double>(button->dataref.c_str(), phase == xplm_CommandBegin ? value : 0.0);
    } else if (button->datarefType == FMCDatarefType::SET_VALUE && phase == xplm_CommandBegin) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        datarefManager->set<double>(button->dataref.c_str(), value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else if (button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE || button->datarefType == FMCDatarefType::EXECUTE_MULTIPLE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

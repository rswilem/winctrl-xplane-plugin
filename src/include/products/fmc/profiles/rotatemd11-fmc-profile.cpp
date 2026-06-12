#include "rotatemd11-fmc-profile.h"

#include "config.h"
#include "dataref.h"
#include "product-fmc.h"

#include <cmath>
#include <cstring>

RotateMD11FMCProfile::RotateMD11FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontMD11);

    const std::string mcdu =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "mcdu_2" : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "mcdu_3"
                                                                                                                                                   : "mcdu_1";
    const std::string brtDataref = "Rotate/aircraft/systems/" + mcdu + "_brt_rat";
    const int idx =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? 1 : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? 2
                                                                                                                                            : 0;

    Dataref::getInstance()->monitorExistingDataref<float>(brtDataref.c_str(), [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("Rotate/aircraft/systems/elec_ac_bus_1_pwrd") ||
                        Dataref::getInstance()->get<bool>("Rotate/aircraft/systems/elec_emer_ac_bus_l_pwrd");
        uint8_t target = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/controls/instr_panel_lts", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("Rotate/aircraft/systems/elec_ac_bus_1_pwrd") ||
                        Dataref::getInstance()->get<bool>("Rotate/aircraft/systems/elec_emer_ac_bus_l_pwrd");
        uint8_t target = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
    }, this);

    // Monitor both power buses and re-trigger brightness callbacks when either changes
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_ac_bus_1_pwrd", [brtDataref](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref(brtDataref.c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/controls/instr_panel_lts");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_emer_ac_bus_l_pwrd", [brtDataref](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref(brtDataref.c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/controls/instr_panel_lts");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>("Rotate/aircraft/systems/mcdu_msg_lt", [product, idx](const std::vector<int> &lights) {
        bool on = idx < static_cast<int>(lights.size()) && lights[idx] > 0;
        product->setLedBrightness(FMCLed::PFP_MSG, on ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_MCDU, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>("Rotate/aircraft/systems/mcdu_dspy_lt", [product, idx](const std::vector<int> &lights) {
        bool on = idx < static_cast<int>(lights.size()) && lights[idx] > 0;
        product->setLedBrightness(FMCLed::PFP_CALL_DISPLAY, on ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_IND, on ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>("Rotate/aircraft/systems/mcdu_fail_lt", [product, idx](const std::vector<int> &lights) {
        bool on = idx < static_cast<int>(lights.size()) && lights[idx] > 0;
        product->setLedBrightness(FMCLed::MCDU_FAIL, on ? 1 : 0);
        product->setLedBrightness(FMCLed::PFP_FAIL, on ? 1 : 0);
    }, this);

    // OFST light
    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>("Rotate/aircraft/systems/mcdu_ofst_lt", [product, idx](const std::vector<int> &lights) {
        bool on = idx < static_cast<int>(lights.size()) && lights[idx] > 0;
        product->setLedBrightness(FMCLed::PFP_OFST, on ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, on ? 1 : 0);
    }, this);

    // Trigger initialization
    Dataref::getInstance()->executeChangedCallbacksForDataref(brtDataref.c_str());
    Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/controls/instr_panel_lts");
    Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/mcdu_msg_lt");
    Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/mcdu_dspy_lt");
    Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/mcdu_fail_lt");
    Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/mcdu_ofst_lt");
}

bool RotateMD11FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/controls/cdu_0/mcdu_line_0_content");
}

bool RotateMD11FMCProfile::shouldReadDatarefAsBytes(const std::string &dataref) const {
    return dataref.find("_style") != std::string::npos;
}

const std::vector<std::string> &RotateMD11FMCProfile::displayDatarefs() const {
    const std::string cdu =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cdu_1" : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "cdu_2"
                                                                                                                                                  : "cdu_0";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_0_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_0_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_1_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_1_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_2_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_2_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_3_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_3_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_4_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_4_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_5_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_5_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_6_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_6_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_7_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_7_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_8_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_8_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_9_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_9_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_10_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_10_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_11_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_11_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_12_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_12_style",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_13_content",
                        "Rotate/aircraft/controls/" + cdu + "/mcdu_line_13_style",
                    })
        .first->second;
}

const std::vector<FMCButtonDef> &RotateMD11FMCProfile::buttonDefs() const {
    const std::string cdu =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cdu_1" : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "cdu_2"
                                                                                                                                                  : "cdu_0";
    const std::string mcdu =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "mcdu_2" : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "mcdu_3"
                                                                                                                                                   : "mcdu_1";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        // Line Select Keys
                        {FMCKey::LSK1L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL1"},
                        {FMCKey::LSK2L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL2"},
                        {FMCKey::LSK3L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL3"},
                        {FMCKey::LSK4L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL4"},
                        {FMCKey::LSK5L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL5"},
                        {FMCKey::LSK6L, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FL6"},
                        {FMCKey::LSK1R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR1"},
                        {FMCKey::LSK2R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR2"},
                        {FMCKey::LSK3R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR3"},
                        {FMCKey::LSK4R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR4"},
                        {FMCKey::LSK5R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR5"},
                        {FMCKey::LSK6R, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FR6"},

                        // Function Keys
                        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_LEGS}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_DIR"},
                        {FMCKey::PROG, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_PROG"},
                        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_PERF"},
                        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP_INIT_REF}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_INIT"},
                        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::PFP_HOLD}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_REF"},
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_ROUTE}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FPLN"},
                        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_NAVRAD"},
                        {std::vector<FMCKey>{FMCKey::MCDU_SEC_FPLN}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_SECFPLN"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_TOAPPR"},
                        {FMCKey::MENU, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_MENU"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_FIX"},
                        {FMCKey::MCDU_EMPTY_TOP_RIGHT, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_ENGOUT"},

                        // Navigation Keys
                        {product->hardwareType == FMCHardwareType::HARDWARE_MCDU ? FMCKey::MCDU_PAGE_DOWN : FMCKey::PAGE_PREV, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_DOWN"},
                        {product->hardwareType == FMCHardwareType::HARDWARE_MCDU ? FMCKey::MCDU_PAGE_UP : FMCKey::PAGE_NEXT, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_UP"},
                        {product->hardwareType == FMCHardwareType::HARDWARE_MCDU ? FMCKey::PAGE_NEXT : FMCKey::PFP_EXEC, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_PAGE"},

                        // Brightness Controls
                        {FMCKey::BRIGHTNESS_UP, "Rotate/aircraft/controls_c/" + mcdu + "_brt_up"},
                        {FMCKey::BRIGHTNESS_DOWN, "Rotate/aircraft/controls_c/" + mcdu + "_brt_dn"},

                        // Numeric Keys
                        {FMCKey::KEY1, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_1"},
                        {FMCKey::KEY2, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_2"},
                        {FMCKey::KEY3, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_3"},
                        {FMCKey::KEY4, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_4"},
                        {FMCKey::KEY5, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_5"},
                        {FMCKey::KEY6, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_6"},
                        {FMCKey::KEY7, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_7"},
                        {FMCKey::KEY8, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_8"},
                        {FMCKey::KEY9, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_9"},
                        {FMCKey::KEY0, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_0"},
                        {FMCKey::PERIOD, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_POINT"},
                        {FMCKey::PLUSMINUS, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_PLUS"},
                        {FMCKey::PFP_DEL, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_MINUS"},

                        // Alpha Keys
                        {FMCKey::KEYA, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_A"},
                        {FMCKey::KEYB, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_B"},
                        {FMCKey::KEYC, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_C"},
                        {FMCKey::KEYD, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_D"},
                        {FMCKey::KEYE, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_E"},
                        {FMCKey::KEYF, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_F"},
                        {FMCKey::KEYG, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_G"},
                        {FMCKey::KEYH, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_H"},
                        {FMCKey::KEYI, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_I"},
                        {FMCKey::KEYJ, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_J"},
                        {FMCKey::KEYK, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_K"},
                        {FMCKey::KEYL, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_L"},
                        {FMCKey::KEYM, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_M"},
                        {FMCKey::KEYN, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_N"},
                        {FMCKey::KEYO, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_O"},
                        {FMCKey::KEYP, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_P"},
                        {FMCKey::KEYQ, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_Q"},
                        {FMCKey::KEYR, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_R"},
                        {FMCKey::KEYS, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_S"},
                        {FMCKey::KEYT, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_T"},
                        {FMCKey::KEYU, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_U"},
                        {FMCKey::KEYV, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_V"},
                        {FMCKey::KEYW, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_W"},
                        {FMCKey::KEYX, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_X"},
                        {FMCKey::KEYY, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_Y"},
                        {FMCKey::KEYZ, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_Z"},

                        // Special Keys
                        {FMCKey::SPACE, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_SPC"},
                        {FMCKey::SLASH, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_BAR"},
                        {FMCKey::CLR, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_CLR"},
                        {FMCKey::MCDU_OVERFLY, "Rotate/aircraft/controls_c/" + cdu + "/mcdu_key_MINUS"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &RotateMD11FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &RotateMD11FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        // Numeric style codes from datarefs
        {1, FMCTextColor::COLOR_GREEN},
        {2, FMCTextColor::COLOR_GREEN},
        {4, FMCTextColor::COLOR_GREEN},
        {5, FMCTextColor::COLOR_GREEN},
        // Character codes for compatibility
        {'w', FMCTextColor::COLOR_GREEN},
        {'W', FMCTextColor::COLOR_GREEN},
        {'g', FMCTextColor::COLOR_GREEN},
        {'G', FMCTextColor::COLOR_GREEN},
        {'e', FMCTextColor::COLOR_GREEN},
        {'E', FMCTextColor::COLOR_GREEN},
    };

    return colMap;
}

std::string RotateMD11FMCProfile::processUTF8Arrows(const std::string &input) {
    // Replace UTF-8 arrow sequences with single-byte ASCII codes
    std::string output;

    for (size_t i = 0; i < input.length();) {
        unsigned char c = static_cast<unsigned char>(input[i]);

        // Check for UTF-8 arrow sequences (0xE2 0x86 0x90-93)
        if (c == 0xE2 && i + 2 < input.length()) {
            unsigned char byte2 = static_cast<unsigned char>(input[i + 1]);
            unsigned char byte3 = static_cast<unsigned char>(input[i + 2]);

            if (byte2 == 0x86) {
                // Arrow characters
                if (byte3 == 0x90) {
                    output += static_cast<char>(28); // Left arrow
                    i += 3;
                } else if (byte3 == 0x92) {
                    output += static_cast<char>(29); // Right arrow
                    i += 3;
                } else if (byte3 == 0x91) {
                    output += static_cast<char>(30); // Up arrow
                    i += 3;
                } else if (byte3 == 0x93) {
                    output += static_cast<char>(31); // Down arrow
                    i += 3;
                } else {
                    i += 3; // Unknown sequence, skip
                }
            } else {
                i += 3; // Unknown sequence, skip
            }
        } else if (c >= 0x80) {
            // Other UTF-8 multi-byte sequence - skip
            if ((c & 0xE0) == 0xC0) {
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                i += 4;
            } else {
                i++;
            }
        } else {
            // Regular ASCII character
            output += c;
            i++;
        }
    }

    return output;
}

void RotateMD11FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '$':
            // Outlined square character
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;
        case '`':
            // Degrees symbol
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case 28: // Left arrow placeholder (will be replaced from UTF-8)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case 29: // Right arrow placeholder (will be replaced from UTF-8)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 30: // Up arrow (ASCII 30 = 0x1E)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 31: // Down arrow (ASCII 31 = 0x1F)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void RotateMD11FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string cdu =
        product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cdu_1" : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "cdu_2"
                                                                                                                                                  : "cdu_0";

    for (int line = 0; line < ProductFMC::PageLines; ++line) {
        std::string contentRef = "Rotate/aircraft/controls/" + cdu + "/mcdu_line_" + std::to_string(line) + "_content";
        std::string styleRef = "Rotate/aircraft/controls/" + cdu + "/mcdu_line_" + std::to_string(line) + "_style";

        std::string contentStr = datarefManager->getCached<std::string>(contentRef.c_str());
        if (contentStr.empty()) {
            continue;
        }

        std::string processedContent = processUTF8Arrows(contentStr);

        std::vector<unsigned char> styleBytes = datarefManager->getCached<std::vector<unsigned char>>(styleRef.c_str());

        for (int pos = 0; pos < ProductFMC::PageCharsPerLine && pos < processedContent.length(); ++pos) {
            unsigned char c = static_cast<unsigned char>(processedContent[pos]);

            if (c == 0x00) {
                continue;
            }

            int styleCode = 4;
            if (pos < styleBytes.size()) {
                styleCode = styleBytes[pos];
            }

            bool fontSmall = (styleCode == 4);

            product->writeLineToPage(page, line, pos, std::string(1, c), 'g', fontSmall);
        }
    }
}

void RotateMD11FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

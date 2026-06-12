#include "fps748-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <regex>

FPS748FMCProfile::FPS748FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    std::string alternatePrefix = isSSG ? "ssg" : "FPS";
    datarefRegex = std::regex(prefix + "/UFMC/LINE_([0-9]+)");

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<float>((alternatePrefix + "/LGT/mcdu_brt_sw").c_str(), [product, alternatePrefix](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>((alternatePrefix + "/Elec/bus_1_powered").c_str()) ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>((alternatePrefix + "/Elec/bus_1_powered").c_str(), [alternatePrefix](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref((alternatePrefix + "/LGT/mcdu_brt_sw").c_str());
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>((prefix + "/UFMC/Exec_Light_on_Pilot").c_str(), [product](bool enabled) {
        if (product->deviceVariant != FMCDeviceVariant::VARIANT_CAPTAIN) {
            return;
        }

        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, enabled ? 1 : 0);
    },
        this);
    Dataref::getInstance()->monitorExistingDataref<bool>((prefix + "/UFMC/Exec_Light_on_Copilot").c_str(), [product](bool enabled) {
        if (product->deviceVariant != FMCDeviceVariant::VARIANT_FIRSTOFFICER) {
            return;
        }

        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, enabled ? 1 : 0);
    },
        this);

    Dataref::getInstance()->executeChangedCallbacksForDataref((alternatePrefix + "/Elec/bus_1_powered").c_str());
}

bool FPS748FMCProfile::IsSSGVersion() { // The older, V2.0 SSG 748
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

bool FPS748FMCProfile::IsFPSVersion() { // The newer, V3.0 FPS 748
    return Dataref::getInstance()->exists("FPS/748/simtime");
}

bool FPS748FMCProfile::IsEligible() {
    return IsFPSVersion() || IsSSGVersion();
}

const std::vector<std::string> &FPS748FMCProfile::displayDatarefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        prefix + "/UFMC/LINE_1",
                        prefix + "/UFMC/LINE_2",
                        prefix + "/UFMC/LINE_3",
                        prefix + "/UFMC/LINE_4",
                        prefix + "/UFMC/LINE_5",
                        prefix + "/UFMC/LINE_6",
                        prefix + "/UFMC/LINE_7",
                        prefix + "/UFMC/LINE_8",
                        prefix + "/UFMC/LINE_9",
                        prefix + "/UFMC/LINE_10",
                        prefix + "/UFMC/LINE_11",
                        prefix + "/UFMC/LINE_12",
                        prefix + "/UFMC/LINE_13",
                        prefix + "/UFMC/LINE_14"})
        .first->second;
}

const std::vector<FMCButtonDef> &FPS748FMCProfile::buttonDefs() const {
    bool isSSG = IsSSGVersion();
    std::string prefix = isSSG ? "SSG" : "FPS";
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cdu1" : "cdu2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, prefix + "/CDU/" + cdu + "_lk1_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2L, prefix + "/CDU/" + cdu + "_lk2_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3L, prefix + "/CDU/" + cdu + "_lk3_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4L, prefix + "/CDU/" + cdu + "_lk4_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5L, prefix + "/CDU/" + cdu + "_lk5_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6L, prefix + "/CDU/" + cdu + "_lk6_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK1R, prefix + "/CDU/" + cdu + "_rk1_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2R, prefix + "/CDU/" + cdu + "_rk2_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3R, prefix + "/CDU/" + cdu + "_rk3_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4R, prefix + "/CDU/" + cdu + "_rk4_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5R, prefix + "/CDU/" + cdu + "_rk5_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6R, prefix + "/CDU/" + cdu + "_rk6_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, prefix + "/CDU/" + cdu + "_init_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, prefix + "/CDU/" + cdu + "_rte_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP4_ATC, FMCKey::MCDU_ATC_COMM}, prefix + "/CDU/" + cdu + "_atc_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP4_VNAV, FMCKey::MCDU_DATA, FMCKey::PFP7_VNAV}, prefix + "/CDU/" + cdu + "_vnav_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::BRIGHTNESS_DOWN, prefix + "/CDU/" + cdu + "_brt_sw", FMCDatarefType::ADJUST_VALUE, -1},
                        {FMCKey::BRIGHTNESS_UP, prefix + "/CDU/" + cdu + "_brt_sw", FMCDatarefType::ADJUST_VALUE, 1},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, prefix + "/CDU/" + cdu + "_fix_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, prefix + "/CDU/" + cdu + "_legs_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, prefix + "/CDU/" + cdu + "_dep_arr_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP_HOLD, prefix + "/CDU/" + cdu + "_hold_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, prefix + "/CDU/" + cdu + "_comm_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PROG, prefix + "/CDU/" + cdu + "_prog_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, prefix + "/CDU/" + cdu + "_exec_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::MENU, prefix + "/CDU/" + cdu + "_menu_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP4_NAV_RAD, FMCKey::MCDU_RAD_NAV, FMCKey::PFP7_NAV_RAD}, prefix + "/CDU/" + cdu + "_radio_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_PREV, prefix + "/CDU/" + cdu + "_prev_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_NEXT, prefix + "/CDU/" + cdu + "_next_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY1, prefix + "/CDU/" + cdu + "_1_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY2, prefix + "/CDU/" + cdu + "_2_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY3, prefix + "/CDU/" + cdu + "_3_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY4, prefix + "/CDU/" + cdu + "_4_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY5, prefix + "/CDU/" + cdu + "_5_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY6, prefix + "/CDU/" + cdu + "_6_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY7, prefix + "/CDU/" + cdu + "_7_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY8, prefix + "/CDU/" + cdu + "_8_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY9, prefix + "/CDU/" + cdu + "_9_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PERIOD, prefix + "/CDU/" + cdu + "_dot_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY0, prefix + "/CDU/" + cdu + "_0_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PLUSMINUS, prefix + "/CDU/" + cdu + "_dash_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYA, prefix + "/CDU/" + cdu + "_a_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYB, prefix + "/CDU/" + cdu + "_b_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYC, prefix + "/CDU/" + cdu + "_c_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYD, prefix + "/CDU/" + cdu + "_d_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYE, prefix + "/CDU/" + cdu + "_e_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYF, prefix + "/CDU/" + cdu + "_f_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYG, prefix + "/CDU/" + cdu + "_g_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYH, prefix + "/CDU/" + cdu + "_h_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYI, prefix + "/CDU/" + cdu + "_i_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYJ, prefix + "/CDU/" + cdu + "_j_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYK, prefix + "/CDU/" + cdu + "_k_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYL, prefix + "/CDU/" + cdu + "_l_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYM, prefix + "/CDU/" + cdu + "_m_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYN, prefix + "/CDU/" + cdu + "_n_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYO, prefix + "/CDU/" + cdu + "_o_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYP, prefix + "/CDU/" + cdu + "_p_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYQ, prefix + "/CDU/" + cdu + "_q_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYR, prefix + "/CDU/" + cdu + "_r_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYS, prefix + "/CDU/" + cdu + "_s_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYT, prefix + "/CDU/" + cdu + "_t_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYU, prefix + "/CDU/" + cdu + "_u_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYV, prefix + "/CDU/" + cdu + "_v_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYW, prefix + "/CDU/" + cdu + "_w_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYX, prefix + "/CDU/" + cdu + "_x_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYY, prefix + "/CDU/" + cdu + "_y_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYZ, prefix + "/CDU/" + cdu + "_z_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SPACE, prefix + "/CDU/" + cdu + "_sp_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, prefix + "/CDU/" + cdu + "_del_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SLASH, prefix + "/CDU/" + cdu + "_slash_sw", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::CLR, prefix + "/CDU/" + cdu + "_clr_sw", FMCDatarefType::SET_VALUE_PHASED}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FPS748FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &FPS748FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'c', FMCTextColor::COLOR_CYAN},
        {'g', FMCTextColor::COLOR_GREEN},
        {'p', FMCTextColor::COLOR_MAGENTA},
        {'w', FMCTextColor::COLOR_WHITE},
        {'l', FMCTextColor::COLOR_RED},                                                                // l = Large/white
        {'s', FMCTextColor::COLOR_GREY},                                                               // s = Small/white
        {'x', FMCTextColor::COLOR_RED},                                                                // x = Special/labels (white)
        {'i', FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_GREY)}, // i = Inverted
    };

    return colMap;
}

void FPS748FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '=':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void FPS748FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (const auto &ref : displayDatarefs()) {
        std::smatch match;
        if (!std::regex_match(ref, match, datarefRegex)) {
            continue;
        }

        int lineNum = std::stoi(match[1]);
        int lineIndex = lineNum - 1;

        if (lineIndex < 0 || lineIndex >= ProductFMC::PageLines) {
            continue;
        }

        std::string text = datarefManager->getCached<std::string>(ref.c_str());
        if (text.empty()) {
            continue;
        }

        // Count visible characters (skip ';X' color escapes, stop at 0x00)
        auto visibleLength = [&](const std::string &s) {
            int len = 0;
            for (int i = 0; i < (int) s.size(); ++i) {
                if ((unsigned char) s[i] == 0x00) {
                    break;
                }
                if (s[i] == ';' && i + 1 < (int) s.size()) {
                    ++i;
                    continue;
                }
                if (s[i] == '[' && i + 1 < (int) s.size() && s[i + 1] == ']') {
                    ++i;
                    ++len;
                    continue;
                }
                ++len;
            }
            return len;
        };

        // If too long, strip spaces from the middle outward until it fits
        while (visibleLength(text) > (int) ProductFMC::PageCharsPerLine) {
            int mid = text.size() / 2;
            int found = -1;
            for (int d = 0; d <= mid; ++d) {
                if (mid + d < (int) text.size() && text[mid + d] == ' ') {
                    found = mid + d;
                    break;
                }
                if (mid - d >= 0 && text[mid - d] == ' ') {
                    found = mid - d;
                    break;
                }
            }
            if (found == -1) {
                break;
            }
            text.erase(found, 1);
        }

        char currentColor = 'W';
        bool fontSmall = lineIndex % 2 == 1;
        int displayPos = 0;

        for (int i = 0; i < text.size() && displayPos < ProductFMC::PageCharsPerLine; ++i) {
            unsigned char c = text[i];
            if (c == 0x00) {
                break;
            }

            if (c == ';' && i + 1 < text.size()) {
                char colorCode = text[i + 1];
                currentColor = colorCode;
                i++; // Skip the color code character
                continue;
            }

            if (c == '[' && i + 1 < text.size() && text[i + 1] == ']') {
                product->writeLineToPage(page, lineIndex, displayPos, "#", currentColor, fontSmall);
                displayPos++;
                i++; // Skip the closing bracket
                continue;
            }

            if (c != 0x20) {
                product->writeLineToPage(page, lineIndex, displayPos, std::string(1, toupper(c)), currentColor, fontSmall);
            }
            displayPos++;
        }
    }
}

void FPS748FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

        if (IsSSGVersion()) {
            // If the dataref contains "cdu1", also execute for "cdu2" because otherwise the SSG/CDU/LINE datarefs don't update.. Strange.
            auto pos = button->dataref.find("cdu1");
            if (pos != std::string::npos) {
                std::string cdu2Dataref = button->dataref;
                cdu2Dataref.replace(pos, 4, "cdu2");
                datarefManager->set<double>(cdu2Dataref.c_str(), phase == xplm_CommandBegin ? value : 0.0);
            }
        }
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

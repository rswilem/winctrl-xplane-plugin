#include "ssg748-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cstring>
#include <regex>

SSG748FMCProfile::SSG748FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    datarefRegex = std::regex("SSG/UFMC/LINE_([0-9]+)");

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontVGA1);

    Dataref::getInstance()->monitorExistingDataref<float>("ssg/LGT/mcdu_brt_sw", [product](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>("ssg/Elec/bus_1_powered") ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("ssg/Elec/bus_1_powered", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("ssg/LGT/mcdu_brt_sw");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("SSG/UFMC/Exec_Light_on_Pilot", [product](bool enabled) {
        if (product->deviceVariant != FMCDeviceVariant::VARIANT_CAPTAIN) {
            return;
        }

        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, enabled ? 1 : 0);
    });
    Dataref::getInstance()->monitorExistingDataref<bool>("SSG/UFMC/Exec_Light_on_Copilot", [product](bool enabled) {
        if (product->deviceVariant != FMCDeviceVariant::VARIANT_FIRSTOFFICER) {
            return;
        }

        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, enabled ? 1 : 0);
    });

    Dataref::getInstance()->executeChangedCallbacksForDataref("ssg/Elec/bus_1_powered");
}

SSG748FMCProfile::~SSG748FMCProfile() {
    Dataref::getInstance()->unbind("ssg/LGT/mcdu_brt_sw");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
}

bool SSG748FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

const std::vector<std::string> &SSG748FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        "SSG/UFMC/LINE_1",
                        "SSG/UFMC/LINE_2",
                        "SSG/UFMC/LINE_3",
                        "SSG/UFMC/LINE_4",
                        "SSG/UFMC/LINE_5",
                        "SSG/UFMC/LINE_6",
                        "SSG/UFMC/LINE_7",
                        "SSG/UFMC/LINE_8",
                        "SSG/UFMC/LINE_9",
                        "SSG/UFMC/LINE_10",
                        "SSG/UFMC/LINE_11",
                        "SSG/UFMC/LINE_12",
                        "SSG/UFMC/LINE_13",
                        "SSG/UFMC/LINE_14"})
        .first->second;
}

const std::vector<FMCButtonDef> &SSG748FMCProfile::buttonDefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cdu1" : "cdu2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "SSG/CDU/" + cdu + "_lk1_sw"},
                        {FMCKey::LSK2L, "SSG/CDU/" + cdu + "_lk2_sw"},
                        {FMCKey::LSK3L, "SSG/CDU/" + cdu + "_lk3_sw"},
                        {FMCKey::LSK4L, "SSG/CDU/" + cdu + "_lk4_sw"},
                        {FMCKey::LSK5L, "SSG/CDU/" + cdu + "_lk5_sw"},
                        {FMCKey::LSK6L, "SSG/CDU/" + cdu + "_lk6_sw"},
                        {FMCKey::LSK1R, "SSG/CDU/" + cdu + "_rk1_sw"},
                        {FMCKey::LSK2R, "SSG/CDU/" + cdu + "_rk2_sw"},
                        {FMCKey::LSK3R, "SSG/CDU/" + cdu + "_rk3_sw"},
                        {FMCKey::LSK4R, "SSG/CDU/" + cdu + "_rk4_sw"},
                        {FMCKey::LSK5R, "SSG/CDU/" + cdu + "_rk5_sw"},
                        {FMCKey::LSK6R, "SSG/CDU/" + cdu + "_rk6_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "SSG/CDU/" + cdu + "_init_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "SSG/CDU/" + cdu + "_rte_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP4_ATC, FMCKey::MCDU_ATC_COMM}, "SSG/CDU/" + cdu + "_atc_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP4_VNAV, FMCKey::MCDU_DATA, FMCKey::PFP7_VNAV}, "SSG/CDU/" + cdu + "_vnav_sw"},
                        {FMCKey::BRIGHTNESS_DOWN, "SSG/CDU/" + cdu + "_brt_sw", -1},
                        {FMCKey::BRIGHTNESS_UP, "SSG/CDU/" + cdu + "_brt_sw", 1},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "SSG/CDU/" + cdu + "_fix_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "SSG/CDU/" + cdu + "_legs_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "SSG/CDU/" + cdu + "_dep_arr_sw"},
                        {FMCKey::PFP_HOLD, "SSG/CDU/" + cdu + "_hold_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, "SSG/CDU/" + cdu + "_comm_sw"},
                        {FMCKey::PROG, "SSG/CDU/" + cdu + "_prog_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "SSG/CDU/" + cdu + "_exec_sw"},
                        {FMCKey::MENU, "SSG/CDU/" + cdu + "_menu_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP4_NAV_RAD, FMCKey::MCDU_RAD_NAV, FMCKey::PFP7_NAV_RAD}, "SSG/CDU/" + cdu + "_radio_sw"},
                        {FMCKey::PAGE_PREV, "SSG/CDU/" + cdu + "_prev_sw"},
                        {FMCKey::PAGE_NEXT, "SSG/CDU/" + cdu + "_next_sw"},
                        {FMCKey::KEY1, "SSG/CDU/" + cdu + "_1_sw"},
                        {FMCKey::KEY2, "SSG/CDU/" + cdu + "_2_sw"},
                        {FMCKey::KEY3, "SSG/CDU/" + cdu + "_3_sw"},
                        {FMCKey::KEY4, "SSG/CDU/" + cdu + "_4_sw"},
                        {FMCKey::KEY5, "SSG/CDU/" + cdu + "_5_sw"},
                        {FMCKey::KEY6, "SSG/CDU/" + cdu + "_6_sw"},
                        {FMCKey::KEY7, "SSG/CDU/" + cdu + "_7_sw"},
                        {FMCKey::KEY8, "SSG/CDU/" + cdu + "_8_sw"},
                        {FMCKey::KEY9, "SSG/CDU/" + cdu + "_9_sw"},
                        {FMCKey::PERIOD, "SSG/CDU/" + cdu + "_dot_sw"},
                        {FMCKey::KEY0, "SSG/CDU/" + cdu + "_0_sw"},
                        {FMCKey::PLUSMINUS, "SSG/CDU/" + cdu + "_dash_sw"},
                        {FMCKey::KEYA, "SSG/CDU/" + cdu + "_a_sw"},
                        {FMCKey::KEYB, "SSG/CDU/" + cdu + "_b_sw"},
                        {FMCKey::KEYC, "SSG/CDU/" + cdu + "_c_sw"},
                        {FMCKey::KEYD, "SSG/CDU/" + cdu + "_d_sw"},
                        {FMCKey::KEYE, "SSG/CDU/" + cdu + "_e_sw"},
                        {FMCKey::KEYF, "SSG/CDU/" + cdu + "_f_sw"},
                        {FMCKey::KEYG, "SSG/CDU/" + cdu + "_g_sw"},
                        {FMCKey::KEYH, "SSG/CDU/" + cdu + "_h_sw"},
                        {FMCKey::KEYI, "SSG/CDU/" + cdu + "_i_sw"},
                        {FMCKey::KEYJ, "SSG/CDU/" + cdu + "_j_sw"},
                        {FMCKey::KEYK, "SSG/CDU/" + cdu + "_k_sw"},
                        {FMCKey::KEYL, "SSG/CDU/" + cdu + "_l_sw"},
                        {FMCKey::KEYM, "SSG/CDU/" + cdu + "_m_sw"},
                        {FMCKey::KEYN, "SSG/CDU/" + cdu + "_n_sw"},
                        {FMCKey::KEYO, "SSG/CDU/" + cdu + "_o_sw"},
                        {FMCKey::KEYP, "SSG/CDU/" + cdu + "_p_sw"},
                        {FMCKey::KEYQ, "SSG/CDU/" + cdu + "_q_sw"},
                        {FMCKey::KEYR, "SSG/CDU/" + cdu + "_r_sw"},
                        {FMCKey::KEYS, "SSG/CDU/" + cdu + "_s_sw"},
                        {FMCKey::KEYT, "SSG/CDU/" + cdu + "_t_sw"},
                        {FMCKey::KEYU, "SSG/CDU/" + cdu + "_u_sw"},
                        {FMCKey::KEYV, "SSG/CDU/" + cdu + "_v_sw"},
                        {FMCKey::KEYW, "SSG/CDU/" + cdu + "_w_sw"},
                        {FMCKey::KEYX, "SSG/CDU/" + cdu + "_x_sw"},
                        {FMCKey::KEYY, "SSG/CDU/" + cdu + "_y_sw"},
                        {FMCKey::KEYZ, "SSG/CDU/" + cdu + "_z_sw"},
                        {FMCKey::SPACE, "SSG/CDU/" + cdu + "_sp_sw"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "SSG/CDU/" + cdu + "_del_sw"},
                        {FMCKey::SLASH, "SSG/CDU/" + cdu + "_slash_sw"},
                        {FMCKey::CLR, "SSG/CDU/" + cdu + "_clr_sw"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &SSG748FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &SSG748FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'c', FMCTextColor::COLOR_CYAN},
        {'g', FMCTextColor::COLOR_GREEN},
        {'p', FMCTextColor::COLOR_MAGENTA},
        {'w', FMCTextColor::COLOR_WHITE},
        {'l', FMCTextColor::COLOR_RED},                                                               // l = Large/white
        {'s', FMCTextColor::COLOR_GREY},                                                              // s = Small/white
        {'x', FMCTextColor::COLOR_RED},                                                               // x = Special/labels (white)
        {'i', FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_RED)}, // i = Inverted
    };

    return colMap;
}

void SSG748FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
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

void SSG748FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
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

void SSG748FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? 1 : 0);

    // If the dataref contains "cdu1", also execute for "cdu2" because the SSG/CDU/LINE datarefs don't update
    auto pos = button->dataref.find("cdu1");
    if (pos != std::string::npos) {
        std::string cdu2Dataref = button->dataref;
        cdu2Dataref.replace(pos, 4, "cdu2");
        Dataref::getInstance()->set<int>(cdu2Dataref.c_str(), phase == xplm_CommandBegin ? 1 : 0);
    }
}

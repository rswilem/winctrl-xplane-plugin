#include "fps-ejets-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>

FPSEjetsFMCProfile::FPSEjetsFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    const std::string prefix = UfmcPrefix();
    datarefRegex = std::regex(prefix + "/UFMC/LINE_([0-9]+)");
    for (int line = 1; line <= 14; line++) {
        lineDatarefs.push_back(prefix + "/UFMC/LINE_" + std::to_string(line));
    }

    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "SSG/CDU/cdu1" : "SSG/CDU/cdu2";
    const std::vector<std::pair<std::variant<FMCKey, std::vector<FMCKey>>, std::string>> keys = {
        {FMCKey::LSK1L, "lk1"},
        {FMCKey::LSK2L, "lk2"},
        {FMCKey::LSK3L, "lk3"},
        {FMCKey::LSK4L, "lk4"},
        {FMCKey::LSK5L, "lk5"},
        {FMCKey::LSK6L, "lk6"},
        {FMCKey::LSK1R, "rk1"},
        {FMCKey::LSK2R, "rk2"},
        {FMCKey::LSK3R, "rk3"},
        {FMCKey::LSK4R, "rk4"},
        {FMCKey::LSK5R, "rk5"},
        {FMCKey::LSK6R, "rk6"},
        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_SEC_FPLN}, "nav"},
        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_INIT}, "rte"},
        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN}, "fpl"},
        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_CLB, FMCKey::PFP3_CRZ, FMCKey::PFP3_DES}, "perf"},
        {std::vector<FMCKey>{FMCKey::MCDU_FUEL_PRED, FMCKey::PFP3_N1_LIMIT}, "trs"},
        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, "dlk"},
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "radio"},
        {FMCKey::PROG, "prog"},
        {FMCKey::MENU, "menu"},
        {FMCKey::PAGE_PREV, "prev"},
        {FMCKey::PAGE_NEXT, "next"},
        {FMCKey::PERIOD, "dot"},
        {FMCKey::PLUSMINUS, "dash"},
        {FMCKey::SPACE, "sp"},
        {FMCKey::SLASH, "slash"},
        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "del"},
        {FMCKey::CLR, "clr"},
    };
    for (const auto &[key, name] : keys) {
        buttons.push_back({key, cdu + "_" + name + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }
    buttons.push_back({FMCKey::BRIGHTNESS_DOWN, cdu + "_brt_sw", FMCDatarefType::SET_VALUE_PHASED, -1});
    buttons.push_back({FMCKey::BRIGHTNESS_UP, cdu + "_brt_sw", FMCDatarefType::SET_VALUE_PHASED, 1});

    const FMCKey digitKeys[] = {FMCKey::KEY0, FMCKey::KEY1, FMCKey::KEY2, FMCKey::KEY3, FMCKey::KEY4, FMCKey::KEY5, FMCKey::KEY6, FMCKey::KEY7, FMCKey::KEY8, FMCKey::KEY9};
    for (int i = 0; i < 10; i++) {
        buttons.push_back({digitKeys[i], cdu + "_" + std::to_string(i) + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }

    const FMCKey letterKeys[] = {FMCKey::KEYA, FMCKey::KEYB, FMCKey::KEYC, FMCKey::KEYD, FMCKey::KEYE, FMCKey::KEYF, FMCKey::KEYG, FMCKey::KEYH, FMCKey::KEYI, FMCKey::KEYJ, FMCKey::KEYK, FMCKey::KEYL, FMCKey::KEYM, FMCKey::KEYN, FMCKey::KEYO, FMCKey::KEYP, FMCKey::KEYQ, FMCKey::KEYR, FMCKey::KEYS, FMCKey::KEYT, FMCKey::KEYU, FMCKey::KEYV, FMCKey::KEYW, FMCKey::KEYX, FMCKey::KEYY, FMCKey::KEYZ};
    for (int i = 0; i < 26; i++) {
        buttons.push_back({letterKeys[i], cdu + "_" + std::string(1, static_cast<char>('a' + i)) + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }

    for (const auto &button : buttons) {
        std::visit([&](auto &&k) {
            using T = std::decay_t<decltype(k)>;
            if constexpr (std::is_same_v<T, FMCKey>) {
                keyMap[k] = &button;
            } else {
                for (const auto &key : k) {
                    keyMap[key] = &button;
                }
            }
        },
            button.key);
    }

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<float>(BrightnessDataref().c_str(), [this](float) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("ssg/Elec/ac_alive", [this](float) {
        updateBacklight();
    },
        this);

    updateBacklight();
}

std::string FPSEjetsFMCProfile::UfmcPrefix() {
    return Dataref::getInstance()->exists("SSG/UFMC/LINE_1") ? "SSG" : "FJCC";
}

std::string FPSEjetsFMCProfile::BrightnessDataref() const {
    return product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "SSG/EJET/LIGHTS/mcdu_p_brt_rt" : "SSG/EJET/LIGHTS/mcdu_cp_brt_rt";
}

void FPSEjetsFMCProfile::updateBacklight() {
    auto datarefManager = Dataref::getInstance();
    const std::string brightnessRef = BrightnessDataref();
    bool powered = !datarefManager->exists("ssg/Elec/ac_alive") || datarefManager->get<float>("ssg/Elec/ac_alive") > 0.5f;
    float brightness = datarefManager->exists(brightnessRef.c_str()) ? datarefManager->get<float>(brightnessRef.c_str()) : 1.0f;
    uint8_t target = powered ? static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255) : 0;

    product->setLedBrightness(FMCLed::BACKLIGHT, target);
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
}

bool FPSEjetsFMCProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

const std::vector<std::string> &FPSEjetsFMCProfile::displayDatarefs() const {
    return lineDatarefs;
}

const std::vector<FMCButtonDef> &FPSEjetsFMCProfile::buttonDefs() const {
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FPSEjetsFMCProfile::buttonKeyMap() const {
    return keyMap;
}

const std::map<char, FMCTextColor> &FPSEjetsFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'c', FMCTextColor::COLOR_CYAN},
        {'g', FMCTextColor::COLOR_GREEN},
        {'p', FMCTextColor::COLOR_MAGENTA},
        {'w', FMCTextColor::COLOR_WHITE},
        {'y', FMCTextColor::COLOR_YELLOW},
        {'l', FMCTextColor::COLOR_RED},
        {'s', FMCTextColor::COLOR_GREY},
        {'x', FMCTextColor::COLOR_RED},
        {'i', FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_GREY)},
    };

    return colMap;
}

void FPSEjetsFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
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

void FPSEjetsFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (const auto &ref : displayDatarefs()) {
        std::smatch match;
        if (!std::regex_match(ref, match, datarefRegex)) {
            continue;
        }

        int lineIndex = std::stoi(match[1]) - 1;
        if (lineIndex < 0 || lineIndex >= ProductFMC::PageLines) {
            continue;
        }

        std::string text = datarefManager->getCached<std::string>(ref.c_str());
        if (text.empty()) {
            continue;
        }

        auto visibleLength = [](const std::string &s) {
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
        const bool lineFontSmall = lineIndex % 2 == 1;
        bool fontSmall = lineFontSmall;
        int displayPos = 0;

        for (int i = 0; i < (int) text.size() && displayPos < ProductFMC::PageCharsPerLine; ++i) {
            unsigned char c = text[i];
            if (c == 0x00) {
                break;
            }

            if (c == ';' && i + 1 < (int) text.size()) {
                char code = text[i + 1];
                if (code == '2') {
                    fontSmall = true;
                } else if (code == '4') {
                    fontSmall = false;
                } else if (!std::isdigit(static_cast<unsigned char>(code))) {
                    currentColor = code;
                    fontSmall = lineFontSmall;
                }
                i++;
                continue;
            }

            if (c == '[' && i + 1 < (int) text.size() && text[i + 1] == ']') {
                product->writeLineToPage(page, lineIndex, displayPos, "#", currentColor, fontSmall);
                displayPos++;
                i++;
                continue;
            }

            if (c != 0x20) {
                product->writeLineToPage(page, lineIndex, displayPos, std::string(1, toupper(c)), currentColor, fontSmall);
            }
            displayPos++;
        }
    }
}

void FPSEjetsFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == FMCDatarefType::SET_VALUE || button->datarefType == FMCDatarefType::SET_VALUE_PHASED) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        if (button->datarefType == FMCDatarefType::SET_VALUE && phase != xplm_CommandBegin) {
            return;
        }

        double phasedValue = phase == xplm_CommandBegin ? value : 0.0;
        datarefManager->set<double>(button->dataref.c_str(), phasedValue);

        if (button->dataref.ends_with("_brt_sw")) {
            return;
        }

        std::string sibling = button->dataref;
        size_t pos = sibling.find("cdu1");
        if (pos != std::string::npos) {
            sibling.replace(pos, 4, "cdu2");
        } else if ((pos = sibling.find("cdu2")) != std::string::npos) {
            sibling.replace(pos, 4, "cdu1");
        } else {
            return;
        }
        datarefManager->set<double>(sibling.c_str(), phasedValue);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::ADJUST_VALUE) {
        double currentValue = datarefManager->get<double>(button->dataref.c_str());
        datarefManager->set<double>(button->dataref.c_str(), currentValue + button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

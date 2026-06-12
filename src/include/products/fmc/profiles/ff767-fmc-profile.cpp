#include "ff767-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <regex>
#include <XPLMDataAccess.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

FlightFactor767FMCProfile::FlightFactor767FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<float>("sim/cockpit/electrical/instrument_brightness", [product](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on") ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/instrument_brightness");
    }, this);
}

bool FlightFactor767FMCProfile::IsEligible() {
    const std::string author = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_author");
    const std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");

    if (!author.starts_with("FlightFactor")) {
        return false;
    }

    static const std::regex icaoPattern("^(B75[23]|B76[234]|76[XY])$");

    return std::regex_match(icao, icaoPattern);
}

const std::vector<std::string> &FlightFactor767FMCProfile::displayDatarefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : "cduR";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        "1-sim/" + cdu + "/display/symbols",        // 336 letters
                        "1-sim/" + cdu + "/display/symbolsColor",   // 336 numbers
                        "1-sim/" + cdu + "/display/symbolsEffects", // 336 numbers
                        "1-sim/" + cdu + "/display/symbolsSize"     // 336 numbers
                    })
        .first->second;
}

const std::vector<FMCButtonDef> &FlightFactor767FMCProfile::buttonDefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "CDU" : "CDU2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "757Avionics/" + cdu + "/LLSK1", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2L, "757Avionics/" + cdu + "/LLSK2", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3L, "757Avionics/" + cdu + "/LLSK3", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4L, "757Avionics/" + cdu + "/LLSK4", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5L, "757Avionics/" + cdu + "/LLSK5", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6L, "757Avionics/" + cdu + "/LLSK6", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK1R, "757Avionics/" + cdu + "/RLSK1", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK2R, "757Avionics/" + cdu + "/RLSK2", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK3R, "757Avionics/" + cdu + "/RLSK3", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK4R, "757Avionics/" + cdu + "/RLSK4", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK5R, "757Avionics/" + cdu + "/RLSK5", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::LSK6R, "757Avionics/" + cdu + "/RLSK6", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "757Avionics/" + cdu + "/init_ref", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "757Avionics/" + cdu + "/rte", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_CLB, "757Avionics/" + cdu + "/clb", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_CRZ, "757Avionics/" + cdu + "/crz", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP3_DES, "757Avionics/" + cdu + "/des", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::BRIGHTNESS_DOWN, "ixeg/733/rheostats/light_fmc_pt_act", FMCDatarefType::ADJUST_VALUE, -0.1},
                        {FMCKey::BRIGHTNESS_UP, "ixeg/733/rheostats/light_fmc_pt_act", FMCDatarefType::ADJUST_VALUE, 0.1},
                        {FMCKey::MENU, "757Avionics/" + cdu + "/mcdu_menu", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "757Avionics/" + cdu + "/legs", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "757Avionics/" + cdu + "/dep_arr", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PFP_HOLD, "757Avionics/" + cdu + "/hold", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PROG, "757Avionics/" + cdu + "/prog", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "757Avionics/" + cdu + "/exec", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_PERF}, "757Avionics/" + cdu + "/dir", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "757Avionics/" + cdu + "/fix", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_PREV, "757Avionics/" + cdu + "/prev_page", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PAGE_NEXT, "757Avionics/" + cdu + "/next_page", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY1, "757Avionics/" + cdu + "/1", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY2, "757Avionics/" + cdu + "/2", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY3, "757Avionics/" + cdu + "/3", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY4, "757Avionics/" + cdu + "/4", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY5, "757Avionics/" + cdu + "/5", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY6, "757Avionics/" + cdu + "/6", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY7, "757Avionics/" + cdu + "/7", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY8, "757Avionics/" + cdu + "/8", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY9, "757Avionics/" + cdu + "/9", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PERIOD, "757Avionics/" + cdu + "/point", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEY0, "757Avionics/" + cdu + "/0", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::PLUSMINUS, "757Avionics/" + cdu + "/plusminus", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYA, "757Avionics/" + cdu + "/A", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYB, "757Avionics/" + cdu + "/B", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYC, "757Avionics/" + cdu + "/C", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYD, "757Avionics/" + cdu + "/D", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYE, "757Avionics/" + cdu + "/E", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYF, "757Avionics/" + cdu + "/F", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYG, "757Avionics/" + cdu + "/G", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYH, "757Avionics/" + cdu + "/H", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYI, "757Avionics/" + cdu + "/I", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYJ, "757Avionics/" + cdu + "/J", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYK, "757Avionics/" + cdu + "/K", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYL, "757Avionics/" + cdu + "/L", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYM, "757Avionics/" + cdu + "/M", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYN, "757Avionics/" + cdu + "/N", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYO, "757Avionics/" + cdu + "/O", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYP, "757Avionics/" + cdu + "/P", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYQ, "757Avionics/" + cdu + "/Q", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYR, "757Avionics/" + cdu + "/R", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYS, "757Avionics/" + cdu + "/S", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYT, "757Avionics/" + cdu + "/T", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYU, "757Avionics/" + cdu + "/U", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYV, "757Avionics/" + cdu + "/V", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYW, "757Avionics/" + cdu + "/W", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYX, "757Avionics/" + cdu + "/X", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYY, "757Avionics/" + cdu + "/Y", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::KEYZ, "757Avionics/" + cdu + "/Z", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SPACE, "757Avionics/" + cdu + "/space", FMCDatarefType::SET_VALUE_PHASED},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "757Avionics/" + cdu + "/delete", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::SLASH, "757Avionics/" + cdu + "/slash", FMCDatarefType::SET_VALUE_PHASED},
                        {FMCKey::CLR, "757Avionics/" + cdu + "/clear", FMCDatarefType::SET_VALUE_PHASED}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FlightFactor767FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &FlightFactor767FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {0, FMCTextColor::COLOR_WHITE},
        {1, FMCTextColor::COLOR_WHITE},
        {2, FMCTextColor::COLOR_MAGENTA},
        {3, FMCTextColor::COLOR_GREEN},
        {4, FMCTextColor::COLOR_CYAN},
        {5, FMCTextColor::COLOR_GREY},
        {6, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_GREY)},
    };
    return colMap;
}

void FlightFactor767FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '*':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void FlightFactor767FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : "cduR";
    std::vector<unsigned char> symbols = datarefManager->getCached<std::vector<unsigned char>>(("1-sim/" + cdu + "/display/symbols").c_str());
    std::vector<int> colors = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsColor").c_str());
    std::vector<int> sizes = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsSize").c_str());
    std::vector<int> effects = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsEffects").c_str());

    if (symbols.size() < FlightFactor767FMCProfile::DataLength || colors.size() < FlightFactor767FMCProfile::DataLength || sizes.size() < FlightFactor767FMCProfile::DataLength || effects.size() < FlightFactor767FMCProfile::DataLength) {
        return;
    }

    for (int line = 0; line < ProductFMC::PageLines && line * ProductFMC::PageCharsPerLine < FlightFactor767FMCProfile::DataLength; ++line) {
        for (int pos = 0; pos < ProductFMC::PageCharsPerLine; ++pos) {
            int index = line * ProductFMC::PageCharsPerLine + pos;

            if (index >= FlightFactor767FMCProfile::DataLength) {
                break;
            }

            char symbol = symbols[index];
            if (symbol == 0x00 || symbol == 0x20) {
                continue;
            }

            unsigned char color = static_cast<unsigned char>(colors[index]);
            unsigned char fontSize = static_cast<unsigned char>(sizes[index]);
            unsigned char effect = static_cast<unsigned char>(effects[index]);
            bool fontSmall = fontSize == 2;

            if (effect == 1) {
                // Inverted text
                color = 6;
            }

            product->writeLineToPage(page, line, pos, std::string(1, symbol), color, fontSmall);
        }
    }
}

void FlightFactor767FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

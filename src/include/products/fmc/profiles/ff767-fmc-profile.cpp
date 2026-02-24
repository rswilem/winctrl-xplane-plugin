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
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/instrument_brightness");
    });
}

FlightFactor767FMCProfile::~FlightFactor767FMCProfile() {
    Dataref::getInstance()->unbind("sim/cockpit/electrical/instrument_brightness");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
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
                        {FMCKey::LSK1L, "757Avionics/" + cdu + "/LLSK1"},
                        {FMCKey::LSK2L, "757Avionics/" + cdu + "/LLSK2"},
                        {FMCKey::LSK3L, "757Avionics/" + cdu + "/LLSK3"},
                        {FMCKey::LSK4L, "757Avionics/" + cdu + "/LLSK4"},
                        {FMCKey::LSK5L, "757Avionics/" + cdu + "/LLSK5"},
                        {FMCKey::LSK6L, "757Avionics/" + cdu + "/LLSK6"},
                        {FMCKey::LSK1R, "757Avionics/" + cdu + "/RLSK1"},
                        {FMCKey::LSK2R, "757Avionics/" + cdu + "/RLSK2"},
                        {FMCKey::LSK3R, "757Avionics/" + cdu + "/RLSK3"},
                        {FMCKey::LSK4R, "757Avionics/" + cdu + "/RLSK4"},
                        {FMCKey::LSK5R, "757Avionics/" + cdu + "/RLSK5"},
                        {FMCKey::LSK6R, "757Avionics/" + cdu + "/RLSK6"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "757Avionics/" + cdu + "/init_ref"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "757Avionics/" + cdu + "/rte"},
                        {FMCKey::PFP3_CLB, "757Avionics/" + cdu + "/clb"},
                        {FMCKey::PFP3_CRZ, "757Avionics/" + cdu + "/crz"},
                        {FMCKey::PFP3_DES, "757Avionics/" + cdu + "/des"},
                        {FMCKey::BRIGHTNESS_DOWN, "ixeg/733/rheostats/light_fmc_pt_act", -0.1},
                        {FMCKey::BRIGHTNESS_UP, "ixeg/733/rheostats/light_fmc_pt_act", 0.1},
                        {FMCKey::MENU, "757Avionics/" + cdu + "/mcdu_menu"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "757Avionics/" + cdu + "/legs"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "757Avionics/" + cdu + "/dep_arr"},
                        {FMCKey::PFP_HOLD, "757Avionics/" + cdu + "/hold"},
                        {FMCKey::PROG, "757Avionics/" + cdu + "/prog"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "757Avionics/" + cdu + "/exec"},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_PERF}, "757Avionics/" + cdu + "/dir"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "757Avionics/" + cdu + "/fix"},
                        {FMCKey::PAGE_PREV, "757Avionics/" + cdu + "/prev_page"},
                        {FMCKey::PAGE_NEXT, "757Avionics/" + cdu + "/next_page"},
                        {FMCKey::KEY1, "757Avionics/" + cdu + "/1"},
                        {FMCKey::KEY2, "757Avionics/" + cdu + "/2"},
                        {FMCKey::KEY3, "757Avionics/" + cdu + "/3"},
                        {FMCKey::KEY4, "757Avionics/" + cdu + "/4"},
                        {FMCKey::KEY5, "757Avionics/" + cdu + "/5"},
                        {FMCKey::KEY6, "757Avionics/" + cdu + "/6"},
                        {FMCKey::KEY7, "757Avionics/" + cdu + "/7"},
                        {FMCKey::KEY8, "757Avionics/" + cdu + "/8"},
                        {FMCKey::KEY9, "757Avionics/" + cdu + "/9"},
                        {FMCKey::PERIOD, "757Avionics/" + cdu + "/point"},
                        {FMCKey::KEY0, "757Avionics/" + cdu + "/0"},
                        {FMCKey::PLUSMINUS, "757Avionics/" + cdu + "/plusminus"},
                        {FMCKey::KEYA, "757Avionics/" + cdu + "/A"},
                        {FMCKey::KEYB, "757Avionics/" + cdu + "/B"},
                        {FMCKey::KEYC, "757Avionics/" + cdu + "/C"},
                        {FMCKey::KEYD, "757Avionics/" + cdu + "/D"},
                        {FMCKey::KEYE, "757Avionics/" + cdu + "/E"},
                        {FMCKey::KEYF, "757Avionics/" + cdu + "/F"},
                        {FMCKey::KEYG, "757Avionics/" + cdu + "/G"},
                        {FMCKey::KEYH, "757Avionics/" + cdu + "/H"},
                        {FMCKey::KEYI, "757Avionics/" + cdu + "/I"},
                        {FMCKey::KEYJ, "757Avionics/" + cdu + "/J"},
                        {FMCKey::KEYK, "757Avionics/" + cdu + "/K"},
                        {FMCKey::KEYL, "757Avionics/" + cdu + "/L"},
                        {FMCKey::KEYM, "757Avionics/" + cdu + "/M"},
                        {FMCKey::KEYN, "757Avionics/" + cdu + "/N"},
                        {FMCKey::KEYO, "757Avionics/" + cdu + "/O"},
                        {FMCKey::KEYP, "757Avionics/" + cdu + "/P"},
                        {FMCKey::KEYQ, "757Avionics/" + cdu + "/Q"},
                        {FMCKey::KEYR, "757Avionics/" + cdu + "/R"},
                        {FMCKey::KEYS, "757Avionics/" + cdu + "/S"},
                        {FMCKey::KEYT, "757Avionics/" + cdu + "/T"},
                        {FMCKey::KEYU, "757Avionics/" + cdu + "/U"},
                        {FMCKey::KEYV, "757Avionics/" + cdu + "/V"},
                        {FMCKey::KEYW, "757Avionics/" + cdu + "/W"},
                        {FMCKey::KEYX, "757Avionics/" + cdu + "/X"},
                        {FMCKey::KEYY, "757Avionics/" + cdu + "/Y"},
                        {FMCKey::KEYZ, "757Avionics/" + cdu + "/Z"},
                        {FMCKey::SPACE, "757Avionics/" + cdu + "/space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "757Avionics/" + cdu + "/delete"},
                        {FMCKey::SLASH, "757Avionics/" + cdu + "/slash"},
                        {FMCKey::CLR, "757Avionics/" + cdu + "/clear"}})
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
        {6, FMCTextColor::COLOR_WHITE_BG}};
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
    if (phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->set<float>(button->dataref.c_str(), phase == xplm_CommandBegin ? 1 : 0);
}

#include "ff777-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cstring>

FlightFactor777FMCProfile::FlightFactor777FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cduR" : "cduC");
    Dataref::getInstance()->monitorExistingDataref<float>(("1-sim/" + cdu + "/brt").c_str(), [product, cdu](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>(("1-sim/" + cdu + "/ok").c_str()) ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<float>("1-sim/ckpt/lights/aisle", [product, cdu](float brightness) {
        uint8_t target = Dataref::getInstance()->get<bool>(("1-sim/" + cdu + "/ok").c_str()) ? brightness * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>(("1-sim/" + cdu + "/ok").c_str(), [cdu](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref(("1-sim/" + cdu + "/brt").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref("1-sim/ckpt/lights/aisle");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lamps/cduCptAct", [product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, enabled ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lamps/cduCptMSG", [product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_MSG, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_MCDU, enabled ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("1-sim/ckpt/lamps/cduCptOFST", [product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_OFST, enabled ? 1 : 0);
    });
}

FlightFactor777FMCProfile::~FlightFactor777FMCProfile() {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cduR" : "cduC");
    Dataref::getInstance()->unbind(("1-sim/" + cdu + "/brt").c_str());
    Dataref::getInstance()->unbind("1-sim/ckpt/lights/aisle");
    Dataref::getInstance()->unbind(("1-sim/" + cdu + "/ok").c_str());
    Dataref::getInstance()->unbind("1-sim/ckpt/lamps/cduCptAct");
    Dataref::getInstance()->unbind("1-sim/ckpt/lamps/cduCptMSG");
    Dataref::getInstance()->unbind("1-sim/ckpt/lamps/cduCptOFST");
}

bool FlightFactor777FMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("1-sim/cduL/display/symbols");
}

const std::vector<std::string> &FlightFactor777FMCProfile::displayDatarefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cduR" : "cduC");
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

const std::vector<FMCButtonDef> &FlightFactor777FMCProfile::buttonDefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cduR" : "cduC");
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "1-sim/command/" + cdu + "LK1_button"},
                        {FMCKey::LSK2L, "1-sim/command/" + cdu + "LK2_button"},
                        {FMCKey::LSK3L, "1-sim/command/" + cdu + "LK3_button"},
                        {FMCKey::LSK4L, "1-sim/command/" + cdu + "LK4_button"},
                        {FMCKey::LSK5L, "1-sim/command/" + cdu + "LK5_button"},
                        {FMCKey::LSK6L, "1-sim/command/" + cdu + "LK6_button"},
                        {FMCKey::LSK1R, "1-sim/command/" + cdu + "RK1_button"},
                        {FMCKey::LSK2R, "1-sim/command/" + cdu + "RK2_button"},
                        {FMCKey::LSK3R, "1-sim/command/" + cdu + "RK3_button"},
                        {FMCKey::LSK4R, "1-sim/command/" + cdu + "RK4_button"},
                        {FMCKey::LSK5R, "1-sim/command/" + cdu + "RK5_button"},
                        {FMCKey::LSK6R, "1-sim/command/" + cdu + "RK6_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "1-sim/command/" + cdu + "initButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "1-sim/command/" + cdu + "rteButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "1-sim/command/" + cdu + "depButton_button"},
                        {FMCKey::PFP7_ALTN, "1-sim/command/" + cdu + "altnButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP7_VNAV, FMCKey::MCDU_DATA, FMCKey::PFP4_VNAV, FMCKey::PFP3_CRZ}, "1-sim/command/" + cdu + "vnavButton_button"},
                        {FMCKey::BRIGHTNESS_DOWN, "1-sim/command/" + cdu + "BrtRotary_rotary-"},
                        {FMCKey::BRIGHTNESS_UP, "1-sim/command/" + cdu + "BrtRotary_rotary+"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "1-sim/command/" + cdu + "fixButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "1-sim/command/" + cdu + "legsButton_button"},
                        {FMCKey::PFP_HOLD, "1-sim/command/" + cdu + "holdButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP7_FMC_COMM, FMCKey::PFP4_FMC_COMM}, "1-sim/command/" + cdu + "fmcCommButton_button"},
                        {FMCKey::PROG, "1-sim/command/" + cdu + "progButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "1-sim/command/" + cdu + "execButton_button"},
                        {FMCKey::MENU, "1-sim/command/" + cdu + "menuButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP7_NAV_RAD, FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD}, "1-sim/command/" + cdu + "navButton_button"},
                        {FMCKey::PAGE_PREV, "1-sim/command/" + cdu + "prevButton_button"},
                        {FMCKey::PAGE_NEXT, "1-sim/command/" + cdu + "nextButton_button"},
                        {FMCKey::KEY1, "1-sim/command/" + cdu + "1Button_button"},
                        {FMCKey::KEY2, "1-sim/command/" + cdu + "2Button_button"},
                        {FMCKey::KEY3, "1-sim/command/" + cdu + "3Button_button"},
                        {FMCKey::KEY4, "1-sim/command/" + cdu + "4Button_button"},
                        {FMCKey::KEY5, "1-sim/command/" + cdu + "5Button_button"},
                        {FMCKey::KEY6, "1-sim/command/" + cdu + "6Button_button"},
                        {FMCKey::KEY7, "1-sim/command/" + cdu + "7Button_button"},
                        {FMCKey::KEY8, "1-sim/command/" + cdu + "8Button_button"},
                        {FMCKey::KEY9, "1-sim/command/" + cdu + "9Button_button"},
                        {FMCKey::PERIOD, "1-sim/command/" + cdu + "dotButton_button"},
                        {FMCKey::KEY0, "1-sim/command/" + cdu + "0Button_button"},
                        {FMCKey::PLUSMINUS, "1-sim/command/" + cdu + "pmButton_button"},
                        {FMCKey::KEYA, "1-sim/command/" + cdu + "AButton_button"},
                        {FMCKey::KEYB, "1-sim/command/" + cdu + "BButton_button"},
                        {FMCKey::KEYC, "1-sim/command/" + cdu + "CButton_button"},
                        {FMCKey::KEYD, "1-sim/command/" + cdu + "DButton_button"},
                        {FMCKey::KEYE, "1-sim/command/" + cdu + "EButton_button"},
                        {FMCKey::KEYF, "1-sim/command/" + cdu + "FButton_button"},
                        {FMCKey::KEYG, "1-sim/command/" + cdu + "GButton_button"},
                        {FMCKey::KEYH, "1-sim/command/" + cdu + "HButton_button"},
                        {FMCKey::KEYI, "1-sim/command/" + cdu + "IButton_button"},
                        {FMCKey::KEYJ, "1-sim/command/" + cdu + "JButton_button"},
                        {FMCKey::KEYK, "1-sim/command/" + cdu + "KButton_button"},
                        {FMCKey::KEYL, "1-sim/command/" + cdu + "LButton_button"},
                        {FMCKey::KEYM, "1-sim/command/" + cdu + "MButton_button"},
                        {FMCKey::KEYN, "1-sim/command/" + cdu + "NButton_button"},
                        {FMCKey::KEYO, "1-sim/command/" + cdu + "OButton_button"},
                        {FMCKey::KEYP, "1-sim/command/" + cdu + "PButton_button"},
                        {FMCKey::KEYQ, "1-sim/command/" + cdu + "QButton_button"},
                        {FMCKey::KEYR, "1-sim/command/" + cdu + "RButton_button"},
                        {FMCKey::KEYS, "1-sim/command/" + cdu + "SButton_button"},
                        {FMCKey::KEYT, "1-sim/command/" + cdu + "TButton_button"},
                        {FMCKey::KEYU, "1-sim/command/" + cdu + "UButton_button"},
                        {FMCKey::KEYV, "1-sim/command/" + cdu + "VButton_button"},
                        {FMCKey::KEYW, "1-sim/command/" + cdu + "WButton_button"},
                        {FMCKey::KEYX, "1-sim/command/" + cdu + "XButton_button"},
                        {FMCKey::KEYY, "1-sim/command/" + cdu + "YButton_button"},
                        {FMCKey::KEYZ, "1-sim/command/" + cdu + "ZButton_button"},
                        {FMCKey::SPACE, "1-sim/command/" + cdu + "spButton_button"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "1-sim/command/" + cdu + "delButton_button"},
                        {FMCKey::SLASH, "1-sim/command/" + cdu + "slashButton_button"},
                        {FMCKey::CLR, "1-sim/command/" + cdu + "clrButton_button"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FlightFactor777FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &FlightFactor777FMCProfile::colorMap() const {
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

void FlightFactor777FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
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

void FlightFactor777FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cduL" : (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "cduR" : "cduC");
    std::vector<unsigned char> symbols = datarefManager->getCached<std::vector<unsigned char>>(("1-sim/" + cdu + "/display/symbols").c_str());
    std::vector<int> colors = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsColor").c_str());
    std::vector<int> sizes = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsSize").c_str());
    std::vector<int> effects = datarefManager->getCached<std::vector<int>>(("1-sim/" + cdu + "/display/symbolsEffects").c_str());

    if (symbols.size() < FlightFactor777FMCProfile::DataLength || colors.size() < FlightFactor777FMCProfile::DataLength || sizes.size() < FlightFactor777FMCProfile::DataLength || effects.size() < FlightFactor777FMCProfile::DataLength) {
        return;
    }

    for (int line = 0; line < ProductFMC::PageLines && line * ProductFMC::PageCharsPerLine < FlightFactor777FMCProfile::DataLength; ++line) {
        for (int pos = 0; pos < ProductFMC::PageCharsPerLine; ++pos) {
            int index = line * ProductFMC::PageCharsPerLine + pos;

            if (index >= FlightFactor777FMCProfile::DataLength) {
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

void FlightFactor777FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

#include "felis-742-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <utility>
#include <vector>

// Felis 747-200 with the FMC navigation fit. The CDU is the stock X-Plane FMS:
// the captain's unit is FMS 1 and the first officer's FMS 2, each with its own
// text/style datarefs and sim/FMS* commands. Keys are mapped by their label.

namespace {
    constexpr const char *kBrightness = "B742/FMC/brightness_ctr";
    // The Felis option for the CDU screen: 0 = colour, 1 = monochrome green
    constexpr const char *kColorOption = "B742/anim/FMC_color_opt";

    // Placeholders for the multi-byte symbols the FMS draws
    constexpr unsigned char kBox = 0x1C;
    constexpr unsigned char kDegrees = 0x1D;
    constexpr unsigned char kArrowUp = 0x1E;
    constexpr unsigned char kArrowDown = 0x1F;
} // namespace

Felis742FMCProfile::Felis742FMCProfile(ProductFMC *product) :
    FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    // Key legends follow the pedestal lighting, the screen its own CDU brightness knob
    auto setKeyBacklight = [product](float brightness) {
        product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    };
    Dataref::getInstance()->monitorExistingDataref<float>("B742/int_light/pedestal_int_light", setKeyBacklight, this);

    // The monitor only reports changes, so dark panel lighting would otherwise keep the product's default brightness
    setKeyBacklight(Dataref::getInstance()->get<float>("B742/int_light/pedestal_int_light"));

    int unit = usesSecondCDU() ? 1 : 0;
    auto updateScreenBrightness = [product, unit]() {
        auto datarefManager = Dataref::getInstance();
        std::vector<float> brightness = datarefManager->getCached<std::vector<float>>(kBrightness);
        bool powered = datarefManager->getCached<bool>("sim/cockpit/electrical/avionics_on");
        float ratio = powered && unit < static_cast<int>(brightness.size()) ? std::clamp(brightness[unit], 0.0f, 1.0f) : 0.0f;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, static_cast<uint8_t>(ratio * 255));
    };

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kBrightness, [updateScreenBrightness](const std::vector<float> &) {
        updateScreenBrightness();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [updateScreenBrightness](bool) {
        updateScreenBrightness();
    },
        this);

    const char *execLight = usesSecondCDU() ? "sim/cockpit2/radios/indicators/fms_exec_light_copilot" : "sim/cockpit2/radios/indicators/fms_exec_light_pilot";
    Dataref::getInstance()->monitorExistingDataref<bool>(execLight, [product](bool lit) {
        product->setLedBrightness(FMCLed::PFP_EXEC, lit ? 1 : 0);
    },
        this);
}

bool Felis742FMCProfile::IsEligible() {
    // The Felis fits one navigation system at a time: 0 = INS, 1 = FMC, 2 = LTN-92
    auto datarefManager = Dataref::getInstance();
    return datarefManager->exists("B742/controls/park_brake_lever") && datarefManager->get<int>("B742/anim/nav_system_select") == 1;
}

bool Felis742FMCProfile::usesSecondCDU() const {
    return product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER;
}

const std::vector<std::string> &Felis742FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const std::string prefix = std::string("sim/cockpit2/radios/indicators/fms_cdu") + (usesSecondCDU() ? "2" : "1");
    std::vector<std::string> datarefs;
    // Only the lines updatePage() draws: a listed ref that is never read stays
    // uncached, and the product keeps force-repainting while any are uncached
    for (int line = 0; line < static_cast<int>(ProductFMC::PageLines); ++line) {
        datarefs.push_back(prefix + "_text_line" + std::to_string(line));
        datarefs.push_back(prefix + "_style_line" + std::to_string(line));
    }
    // Repaint when the screen is switched between colour and green
    datarefs.push_back(kColorOption);

    return cache.emplace(product->deviceVariant, std::move(datarefs)).first->second;
}

const std::vector<FMCButtonDef> &Felis742FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const std::string fms = usesSecondCDU() ? "sim/FMS2/" : "sim/FMS/";

    return cache.emplace(product->deviceVariant,
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
                        // PFP4/PFP7 have no CLB/CRZ/DES; VNAV opens the climb page
                        {std::vector<FMCKey>{FMCKey::PFP3_CLB, FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV, FMCKey::MCDU_PERF}, fms + "clb"},
                        {FMCKey::PFP3_CRZ, fms + "crz"},
                        {FMCKey::PFP3_DES, fms + "des"},
                        {std::vector<FMCKey>{FMCKey::MENU, FMCKey::MCDU_DIR}, fms + "dir_intc"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN}, fms + "legs"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, fms + "dep_arr"},
                        {FMCKey::PFP_HOLD, fms + "hold"},
                        {FMCKey::PROG, fms + "prog"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, fms + "exec"},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD, FMCKey::MCDU_RAD_NAV}, fms + "navrad"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, fms + "fix"},
                        {FMCKey::PAGE_PREV, fms + "prev"},
                        {FMCKey::PAGE_NEXT, fms + "next"},
                        {FMCKey::MCDU_PAGE_UP, fms + "up"},
                        {FMCKey::MCDU_PAGE_DOWN, fms + "down"},
                        {FMCKey::BRIGHTNESS_DOWN, kBrightness, FMCDatarefType::ADJUST_VALUE, -0.1},
                        {FMCKey::BRIGHTNESS_UP, kBrightness, FMCDatarefType::ADJUST_VALUE, 0.1},
                        {FMCKey::KEY1, fms + "key_1"},
                        {FMCKey::KEY2, fms + "key_2"},
                        {FMCKey::KEY3, fms + "key_3"},
                        {FMCKey::KEY4, fms + "key_4"},
                        {FMCKey::KEY5, fms + "key_5"},
                        {FMCKey::KEY6, fms + "key_6"},
                        {FMCKey::KEY7, fms + "key_7"},
                        {FMCKey::KEY8, fms + "key_8"},
                        {FMCKey::KEY9, fms + "key_9"},
                        {FMCKey::PERIOD, fms + "key_period"},
                        {FMCKey::KEY0, fms + "key_0"},
                        {FMCKey::PLUSMINUS, fms + "key_minus"},
                        {FMCKey::KEYA, fms + "key_A"},
                        {FMCKey::KEYB, fms + "key_B"},
                        {FMCKey::KEYC, fms + "key_C"},
                        {FMCKey::KEYD, fms + "key_D"},
                        {FMCKey::KEYE, fms + "key_E"},
                        {FMCKey::KEYF, fms + "key_F"},
                        {FMCKey::KEYG, fms + "key_G"},
                        {FMCKey::KEYH, fms + "key_H"},
                        {FMCKey::KEYI, fms + "key_I"},
                        {FMCKey::KEYJ, fms + "key_J"},
                        {FMCKey::KEYK, fms + "key_K"},
                        {FMCKey::KEYL, fms + "key_L"},
                        {FMCKey::KEYM, fms + "key_M"},
                        {FMCKey::KEYN, fms + "key_N"},
                        {FMCKey::KEYO, fms + "key_O"},
                        {FMCKey::KEYP, fms + "key_P"},
                        {FMCKey::KEYQ, fms + "key_Q"},
                        {FMCKey::KEYR, fms + "key_R"},
                        {FMCKey::KEYS, fms + "key_S"},
                        {FMCKey::KEYT, fms + "key_T"},
                        {FMCKey::KEYU, fms + "key_U"},
                        {FMCKey::KEYV, fms + "key_V"},
                        {FMCKey::KEYW, fms + "key_W"},
                        {FMCKey::KEYX, fms + "key_X"},
                        {FMCKey::KEYY, fms + "key_Y"},
                        {FMCKey::KEYZ, fms + "key_Z"},
                        {FMCKey::SPACE, fms + "key_space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, fms + "key_delete"},
                        {FMCKey::SLASH, fms + "key_slash"},
                        {FMCKey::CLR, fms + "key_clear"},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &Felis742FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &Felis742FMCProfile::colorMap() const {
    // Style byte bits 0-2 are the colour and bit 6 inverse video (bit 7, the large font, is split off)
    static const std::map<char, FMCTextColor> colourScreen = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x02, FMCTextColor::COLOR_RED},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_MAGENTA},
        {0x06, FMCTextColor::COLOR_AMBER},
        {0x07, FMCTextColor::COLOR_WHITE},
        {0x47, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},
    };

    // The hardware palette has a single green, so the green screen loses the
    // Felis' light/dark shading between white and cyan text
    static const std::map<char, FMCTextColor> greenScreen = {
        {0x00, FMCTextColor::COLOR_GREEN},
        {0x01, FMCTextColor::COLOR_GREEN},
        {0x02, FMCTextColor::COLOR_GREEN},
        {0x03, FMCTextColor::COLOR_GREEN},
        {0x04, FMCTextColor::COLOR_GREEN},
        {0x05, FMCTextColor::COLOR_GREEN},
        {0x06, FMCTextColor::COLOR_GREEN},
        {0x07, FMCTextColor::COLOR_GREEN},
        {0x47, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_GREEN)},
    };

    bool isGreenScreen = Dataref::getInstance()->getCached<int>(kColorOption) == 1;
    return isGreenScreen ? greenScreen : colourScreen;
}

void Felis742FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case kBox:
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case kDegrees:
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case kArrowUp:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case kArrowDown:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void Felis742FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    const std::string prefix = std::string("sim/cockpit2/radios/indicators/fms_cdu") + (usesSecondCDU() ? "2" : "1");
    const std::vector<std::pair<std::string, unsigned char>> symbols = {
        {"\u2610", kBox},
        {"\u00B0", kDegrees},
        {"\u2191", kArrowUp},
        {"\u2193", kArrowDown},
        {"\u25c0", '<'},
        {"\u25b6", '>'},
    };

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

        for (int i = 0; i < static_cast<int>(text.size()) && i < static_cast<int>(ProductFMC::PageCharsPerLine); ++i) {
            char c = text[i];
            if (c == 0x00) {
                continue;
            }
            if (static_cast<unsigned char>(c) > 127) {
                c = '?';
            }

            unsigned char style = i < static_cast<int>(styleBytes.size()) ? styleBytes[i] : 0x00;
            bool fontSmall = (style & 0x80) == 0;
            product->writeLineToPage(page, lineNum, i, std::string(1, c), style & 0x47, fontSmall);
        }
    }
}

void Felis742FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == FMCDatarefType::ADJUST_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        // One brightness knob per CDU
        std::vector<float> brightness = datarefManager->get<std::vector<float>>(button->dataref.c_str());
        size_t unit = usesSecondCDU() ? 1 : 0;
        if (unit < brightness.size()) {
            brightness[unit] = std::clamp(brightness[unit] + static_cast<float>(button->value), 0.0f, 1.0f);
            datarefManager->set<std::vector<float>>(button->dataref.c_str(), brightness);
        }
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

#include "zibo-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

ZiboFMCProfile::ZiboFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    datarefRegex = std::regex("laminar/B738/fmc[0-9]+/Line([0-9]{2})_([A-Z]+)");

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/instrument_brightness", [product](std::vector<float> screenBrightness) {
        if (screenBrightness.size() < 11) {
            return;
        }

        uint8_t target = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on") ? screenBrightness[product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? 10 : 11] * 255 : 0;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [product](std::vector<float> panelBrightness) {
        if (panelBrightness.size() < 4) {
            return;
        }

        uint8_t target = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on") ? panelBrightness[3] * 255 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/instrument_brightness");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/fmc/fmc_message", [product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_MSG, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_MCDU, enabled ? 1 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/indicators/fmc_exec_lights", [product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_EXEC, enabled ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_RDY, enabled ? 1 : 0);
    });
}

ZiboFMCProfile::~ZiboFMCProfile() {
    Dataref::getInstance()->unbind("laminar/B738/electric/instrument_brightness");
    Dataref::getInstance()->unbind("laminar/B738/electric/panel_brightness");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("laminar/B738/fmc/fmc_message");
    Dataref::getInstance()->unbind("laminar/B738/indicators/fmc_exec_lights");
}

bool ZiboFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/electric/instrument_brightness");
}

const std::vector<std::string> &ZiboFMCProfile::displayDatarefs() const {
    const std::string fmc = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "fmc1" : "fmc2";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<std::string>{
                        "laminar/B738/" + fmc + "/Line00_C",
                        "laminar/B738/" + fmc + "/Line00_G",
                        "laminar/B738/" + fmc + "/Line00_I",
                        "laminar/B738/" + fmc + "/Line00_L",
                        "laminar/B738/" + fmc + "/Line00_M",
                        "laminar/B738/" + fmc + "/Line00_S",

                        "laminar/B738/" + fmc + "/Line01_G",
                        "laminar/B738/" + fmc + "/Line01_GX",
                        "laminar/B738/" + fmc + "/Line01_I",
                        "laminar/B738/" + fmc + "/Line01_L",
                        "laminar/B738/" + fmc + "/Line01_LX",
                        "laminar/B738/" + fmc + "/Line01_M",
                        "laminar/B738/" + fmc + "/Line01_S",
                        "laminar/B738/" + fmc + "/Line01_X",

                        "laminar/B738/" + fmc + "/Line02_G",
                        "laminar/B738/" + fmc + "/Line02_GX",
                        "laminar/B738/" + fmc + "/Line02_I",
                        "laminar/B738/" + fmc + "/Line02_L",
                        "laminar/B738/" + fmc + "/Line02_LX",
                        "laminar/B738/" + fmc + "/Line02_M",
                        "laminar/B738/" + fmc + "/Line02_S",
                        "laminar/B738/" + fmc + "/Line02_X",

                        "laminar/B738/" + fmc + "/Line03_G",
                        "laminar/B738/" + fmc + "/Line03_GX",
                        "laminar/B738/" + fmc + "/Line03_I",
                        "laminar/B738/" + fmc + "/Line03_L",
                        "laminar/B738/" + fmc + "/Line03_LX",
                        "laminar/B738/" + fmc + "/Line03_M",
                        "laminar/B738/" + fmc + "/Line03_S",
                        "laminar/B738/" + fmc + "/Line03_X",

                        "laminar/B738/" + fmc + "/Line04_G",
                        "laminar/B738/" + fmc + "/Line04_GX",
                        "laminar/B738/" + fmc + "/Line04_I",
                        "laminar/B738/" + fmc + "/Line04_L",
                        "laminar/B738/" + fmc + "/Line04_LX",
                        "laminar/B738/" + fmc + "/Line04_M",
                        "laminar/B738/" + fmc + "/Line04_S",
                        "laminar/B738/" + fmc + "/Line04_SI",
                        "laminar/B738/" + fmc + "/Line04_X",

                        "laminar/B738/" + fmc + "/Line05_G",
                        "laminar/B738/" + fmc + "/Line05_GX",
                        "laminar/B738/" + fmc + "/Line05_I",
                        "laminar/B738/" + fmc + "/Line05_L",
                        "laminar/B738/" + fmc + "/Line05_LX",
                        "laminar/B738/" + fmc + "/Line05_M",
                        "laminar/B738/" + fmc + "/Line05_S",
                        "laminar/B738/" + fmc + "/Line05_X",

                        "laminar/B738/" + fmc + "/Line06_G",
                        "laminar/B738/" + fmc + "/Line06_GX",
                        "laminar/B738/" + fmc + "/Line06_I",
                        "laminar/B738/" + fmc + "/Line06_L",
                        "laminar/B738/" + fmc + "/Line06_LX",
                        "laminar/B738/" + fmc + "/Line06_M",
                        "laminar/B738/" + fmc + "/Line06_S",
                        "laminar/B738/" + fmc + "/Line06_X",

                        "laminar/B738/" + fmc + "/Line_entry",
                        "laminar/B738/" + fmc + "/Line_entry_I"})
        .first->second;
}

const std::vector<FMCButtonDef> &ZiboFMCProfile::buttonDefs() const {
    const std::string fmc = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "fmc1" : "fmc2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "laminar/B738/button/" + fmc + "_1L"},
                        {FMCKey::LSK2L, "laminar/B738/button/" + fmc + "_2L"},
                        {FMCKey::LSK3L, "laminar/B738/button/" + fmc + "_3L"},
                        {FMCKey::LSK4L, "laminar/B738/button/" + fmc + "_4L"},
                        {FMCKey::LSK5L, "laminar/B738/button/" + fmc + "_5L"},
                        {FMCKey::LSK6L, "laminar/B738/button/" + fmc + "_6L"},
                        {FMCKey::LSK1R, "laminar/B738/button/" + fmc + "_1R"},
                        {FMCKey::LSK2R, "laminar/B738/button/" + fmc + "_2R"},
                        {FMCKey::LSK3R, "laminar/B738/button/" + fmc + "_3R"},
                        {FMCKey::LSK4R, "laminar/B738/button/" + fmc + "_4R"},
                        {FMCKey::LSK5R, "laminar/B738/button/" + fmc + "_5R"},
                        {FMCKey::LSK6R, "laminar/B738/button/" + fmc + "_6R"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "laminar/B738/button/" + fmc + "_init_ref"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "laminar/B738/button/" + fmc + "_rte"},
                        {FMCKey::PFP3_CLB, "laminar/B738/button/" + fmc + "_clb"},
                        {FMCKey::PFP3_CRZ, "laminar/B738/button/" + fmc + "_crz"},
                        {FMCKey::PFP3_DES, "laminar/B738/button/" + fmc + "_des"},
                        {FMCKey::BRIGHTNESS_DOWN, "laminar/B738/electric/instrument_brightness[10]", -0.1},
                        {FMCKey::BRIGHTNESS_UP, "laminar/B738/electric/instrument_brightness[10]", 0.1},
                        {FMCKey::MENU, "laminar/B738/button/" + fmc + "_menu"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "laminar/B738/button/" + fmc + "_legs"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "laminar/B738/button/" + fmc + "_dep_app"},
                        {FMCKey::PFP_HOLD, "laminar/B738/button/" + fmc + "_hold"},
                        {FMCKey::PROG, "laminar/B738/button/" + fmc + "_prog"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "laminar/B738/button/" + fmc + "_exec"},
                        {std::vector<FMCKey>{FMCKey::PFP3_N1_LIMIT, FMCKey::MCDU_PERF}, "laminar/B738/button/" + fmc + "_n1_lim"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "laminar/B738/button/" + fmc + "_fix"},
                        {FMCKey::PAGE_PREV, "laminar/B738/button/" + fmc + "_prev_page"},
                        {FMCKey::PAGE_NEXT, "laminar/B738/button/" + fmc + "_next_page"},
                        {FMCKey::KEY1, "laminar/B738/button/" + fmc + "_1"},
                        {FMCKey::KEY2, "laminar/B738/button/" + fmc + "_2"},
                        {FMCKey::KEY3, "laminar/B738/button/" + fmc + "_3"},
                        {FMCKey::KEY4, "laminar/B738/button/" + fmc + "_4"},
                        {FMCKey::KEY5, "laminar/B738/button/" + fmc + "_5"},
                        {FMCKey::KEY6, "laminar/B738/button/" + fmc + "_6"},
                        {FMCKey::KEY7, "laminar/B738/button/" + fmc + "_7"},
                        {FMCKey::KEY8, "laminar/B738/button/" + fmc + "_8"},
                        {FMCKey::KEY9, "laminar/B738/button/" + fmc + "_9"},
                        {FMCKey::PERIOD, "laminar/B738/button/" + fmc + "_period"},
                        {FMCKey::KEY0, "laminar/B738/button/" + fmc + "_0"},
                        {FMCKey::PLUSMINUS, "laminar/B738/button/" + fmc + "_minus"},
                        {FMCKey::KEYA, "laminar/B738/button/" + fmc + "_A"},
                        {FMCKey::KEYB, "laminar/B738/button/" + fmc + "_B"},
                        {FMCKey::KEYC, "laminar/B738/button/" + fmc + "_C"},
                        {FMCKey::KEYD, "laminar/B738/button/" + fmc + "_D"},
                        {FMCKey::KEYE, "laminar/B738/button/" + fmc + "_E"},
                        {FMCKey::KEYF, "laminar/B738/button/" + fmc + "_F"},
                        {FMCKey::KEYG, "laminar/B738/button/" + fmc + "_G"},
                        {FMCKey::KEYH, "laminar/B738/button/" + fmc + "_H"},
                        {FMCKey::KEYI, "laminar/B738/button/" + fmc + "_I"},
                        {FMCKey::KEYJ, "laminar/B738/button/" + fmc + "_J"},
                        {FMCKey::KEYK, "laminar/B738/button/" + fmc + "_K"},
                        {FMCKey::KEYL, "laminar/B738/button/" + fmc + "_L"},
                        {FMCKey::KEYM, "laminar/B738/button/" + fmc + "_M"},
                        {FMCKey::KEYN, "laminar/B738/button/" + fmc + "_N"},
                        {FMCKey::KEYO, "laminar/B738/button/" + fmc + "_O"},
                        {FMCKey::KEYP, "laminar/B738/button/" + fmc + "_P"},
                        {FMCKey::KEYQ, "laminar/B738/button/" + fmc + "_Q"},
                        {FMCKey::KEYR, "laminar/B738/button/" + fmc + "_R"},
                        {FMCKey::KEYS, "laminar/B738/button/" + fmc + "_S"},
                        {FMCKey::KEYT, "laminar/B738/button/" + fmc + "_T"},
                        {FMCKey::KEYU, "laminar/B738/button/" + fmc + "_U"},
                        {FMCKey::KEYV, "laminar/B738/button/" + fmc + "_V"},
                        {FMCKey::KEYW, "laminar/B738/button/" + fmc + "_W"},
                        {FMCKey::KEYX, "laminar/B738/button/" + fmc + "_X"},
                        {FMCKey::KEYY, "laminar/B738/button/" + fmc + "_Y"},
                        {FMCKey::KEYZ, "laminar/B738/button/" + fmc + "_Z"},
                        {FMCKey::SPACE, "laminar/B738/button/" + fmc + "_SP"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "laminar/B738/button/" + fmc + "_del"},
                        {FMCKey::SLASH, "laminar/B738/button/" + fmc + "_slash"},
                        {FMCKey::CLR, "laminar/B738/button/" + fmc + "_clr"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &ZiboFMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &ZiboFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'W', FMCTextColor::COLOR_WHITE},
        {'L', FMCTextColor::COLOR_WHITE},
        {'S', FMCTextColor::COLOR_WHITE}, // White, small
        {'M', FMCTextColor::COLOR_MAGENTA},
        {'G', FMCTextColor::COLOR_GREEN},
        {'C', FMCTextColor::COLOR_CYAN},
        {'I', FMCTextColor::COLOR_WHITE_BG}, // White (should be inverted gray/white)
        {'X', FMCTextColor::COLOR_WHITE},    // White (should be special labels)
    };

    return colMap;
}

void ZiboFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '*':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void ZiboFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    for (const auto &ref : displayDatarefs()) {
        std::string text = datarefManager->getCached<std::string>(ref.c_str());

        // Handle scratchpad datarefs specially
        if (ref.ends_with("/Line_entry") || ref.ends_with("/Line_entry_I")) {
            if (!text.empty()) {
                const std::string fmc = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "fmc1" : "fmc2";
                char color = (ref == "laminar/B738/" + fmc + "/Line_entry_I") ? 'I' : 'W';

                // Store scratchpad text for later display on line 13
                for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
                    char c = text[i];
                    if (c == 0x00) {
                        break; // End of string
                    }
                    if (c != 0x20) { // Skip spaces
                        product->writeLineToPage(page, 13, i, std::string(1, c), color, false);
                    }
                }
            }
            continue;
        }

        std::smatch match;
        if (!std::regex_match(ref, match, datarefRegex)) {
            continue;
        }

        unsigned char lineNum = std::stoi(match[1]);
        std::string colorStr = match[2];

        // For double-letter codes like "GX", "LX", use first letter for color
        char color = colorStr[0];

        unsigned char displayLine = lineNum * 2;
        bool fontSmall = color == 'X' || color == 'S';
        if (colorStr.back() == 'X') {
            displayLine -= 1; // X datarefs go to odd lines (labels)
        }

        if (text.empty()) {
            continue;
        }

        for (int i = 0; i < text.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            char c = text[i];
            if (c == 0x00) {
                break;
            }

            if (c != 0x20) {
                product->writeLineToPage(page, displayLine, i, std::string(1, c), color, fontSmall);
            }
        }
    }
}

void ZiboFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (std::fabs(button->value) > std::numeric_limits<double>::epsilon()) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        std::string ref = button->dataref;
        size_t start = ref.find('[');
        if (start != std::string::npos) {
            size_t end = ref.find(']', start);
            int index = std::stoi(ref.substr(start + 1, end - start - 1));
            std::string baseRef = ref.substr(0, start);

            auto vec = Dataref::getInstance()->get<std::vector<float>>(baseRef.c_str());
            if (index >= 0 && index < (int) vec.size()) {
                vec[index] = std::clamp(vec[index] + button->value, 0.0, 1.0);
                Dataref::getInstance()->set<std::vector<float>>(baseRef.c_str(), vec);
            }
        } else {
            Dataref::getInstance()->set<float>(ref.c_str(), button->value);
        }
    } else if (Dataref::getInstance()->get<int>("laminar/B738/fmc_type") == 1) {
        std::vector<std::pair<FMCKey, FMCKey>> fansMapping = {
            {FMCKey::PFP_DEP_ARR, FMCKey::PFP3_CLB},
            {FMCKey::PFP4_ATC, FMCKey::PFP3_CRZ},
            {FMCKey::PFP4_VNAV, FMCKey::PFP3_DES},
            {FMCKey::PFP7_VNAV, FMCKey::PFP3_DES},
            {FMCKey::PFP_FIX, FMCKey::MENU},
            {FMCKey::PFP_HOLD, FMCKey::PFP_DEP_ARR},
            {FMCKey::PFP4_FMC_COMM, FMCKey::PFP_HOLD},
            {FMCKey::PFP7_FMC_COMM, FMCKey::PFP_HOLD},
            {FMCKey::MENU, FMCKey::PFP3_N1_LIMIT},
            {FMCKey::PFP3_N1_LIMIT, FMCKey::PFP_FIX},
        };

        // We should read the pressed key, and then "convert" the key to the mapped one, if needed. After conversion, execute the button command dataref. Else, just execute
        FMCKey pressedKey = button->key.index() == 0 ? std::get<FMCKey>(button->key) : std::get<std::vector<FMCKey>>(button->key)[0];

        for (const auto &mapping : fansMapping) {
            if (pressedKey == mapping.first) {
                // Find the mapped button
                auto it = buttonKeyMap().find(mapping.second);
                if (it != buttonKeyMap().end()) {
                    Dataref::getInstance()->executeCommand(it->second->dataref.c_str(), phase);
                    return;
                }
            }
        }

        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    } else {
        Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
    }
}

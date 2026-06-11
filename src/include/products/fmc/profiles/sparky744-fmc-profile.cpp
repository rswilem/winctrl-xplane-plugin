#include "sparky744-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

SparkyB744FMCProfile::SparkyB744FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font744);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [product](bool powered) {
        uint8_t target = powered ? 200 : 0;
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/radios/indicators/fms_exec_light_pilot", [product](bool lit) {
        if (product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN) {
            uint8_t target = lit ? 200 : 0;
            product->setLedBrightness(FMCLed::PFP_EXEC, target);
            product->setLedBrightness(FMCLed::MCDU_STATUS, target);
        }
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/radios/indicators/fms_exec_light_copilot", [product](bool lit) {
        if (product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER) {
            uint8_t target = lit ? 200 : 0;
            product->setLedBrightness(FMCLed::PFP_EXEC, target);
            product->setLedBrightness(FMCLed::MCDU_STATUS, target);
        }
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/radios/indicators/fms_exec_light_pilot");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/radios/indicators/fms_exec_light_copilot");
}

SparkyB744FMCProfile::~SparkyB744FMCProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool SparkyB744FMCProfile::IsEligible() {
    auto dr = Dataref::getInstance();
    return dr->exists("laminar/B747/fms1/Line01_L") && !dr->exists("FPS/748/simtime") && !dr->exists("SSG/748/simtime");
}

const std::vector<std::string> &SparkyB744FMCProfile::displayDatarefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;
    // B747 quirk: captain CDU keys use fms1 but display lines come from fms3 (observer FMS). FO uses fms2 for both. Observer uses fms1 display lines.
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms3"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "fms1"
                                                                                         : "fms2";

    std::vector<std::string> refs;
    refs.reserve(28);
    for (int i = 1; i <= 14; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d", i);
        refs.push_back("laminar/B747/" + fms + "/Line" + buf + "_L");
        refs.push_back("laminar/B747/" + fms + "/Line" + buf + "_S");
    }
    return cache.try_emplace(product->deviceVariant, std::move(refs)).first->second;
}

const std::vector<FMCButtonDef> &SparkyB744FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;
    // LSK keys: captain=fms1, FO=fms2, observer=fms3
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms1"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "fms3"
                                                                                         : "fms2";

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "laminar/B747/" + fms + "/ls_key/L1"},
                        {FMCKey::LSK2L, "laminar/B747/" + fms + "/ls_key/L2"},
                        {FMCKey::LSK3L, "laminar/B747/" + fms + "/ls_key/L3"},
                        {FMCKey::LSK4L, "laminar/B747/" + fms + "/ls_key/L4"},
                        {FMCKey::LSK5L, "laminar/B747/" + fms + "/ls_key/L5"},
                        {FMCKey::LSK6L, "laminar/B747/" + fms + "/ls_key/L6"},
                        {FMCKey::LSK1R, "laminar/B747/" + fms + "/ls_key/R1"},
                        {FMCKey::LSK2R, "laminar/B747/" + fms + "/ls_key/R2"},
                        {FMCKey::LSK3R, "laminar/B747/" + fms + "/ls_key/R3"},
                        {FMCKey::LSK4R, "laminar/B747/" + fms + "/ls_key/R4"},
                        {FMCKey::LSK5R, "laminar/B747/" + fms + "/ls_key/R5"},
                        {FMCKey::LSK6R, "laminar/B747/" + fms + "/ls_key/R6"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "laminar/B747/" + fms + "/func_key/index"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "laminar/B747/" + fms + "/func_key/fpln"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "laminar/B747/" + fms + "/func_key/clb"},
                        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, "laminar/B747/" + fms + "/func_key/crz"},
                        {std::vector<FMCKey>{FMCKey::PFP4_VNAV, FMCKey::MCDU_DATA, FMCKey::PFP7_VNAV}, "laminar/B747/" + fms + "/func_key/des"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "laminar/B747/" + fms + "/func_key/dir_intc"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "laminar/B747/" + fms + "/func_key/legs"},
                        {FMCKey::PFP_HOLD, "laminar/B747/" + fms + "/func_key/dep_arr"},
                        {std::vector<FMCKey>{FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM, FMCKey::MCDU_DATA}, "laminar/B747/" + fms + "/func_key/hold"},
                        {FMCKey::PROG, "laminar/B747/" + fms + "/func_key/prog"},
                        {FMCKey::MENU, "laminar/B747/" + fms + "/func_key/fix"},
                        {std::vector<FMCKey>{FMCKey::PFP4_NAV_RAD, FMCKey::MCDU_RAD_NAV, FMCKey::PFP7_NAV_RAD}, "laminar/B747/" + fms + "/func_key/navrad"},
                        {FMCKey::PAGE_PREV, "laminar/B747/" + fms + "/func_key/prev_pg"},
                        {FMCKey::PAGE_NEXT, "laminar/B747/" + fms + "/func_key/next_pg"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "laminar/B747/" + fms + "/key/execute"},
                        {FMCKey::KEY1, "laminar/B747/" + fms + "/alphanum_key/1"},
                        {FMCKey::KEY2, "laminar/B747/" + fms + "/alphanum_key/2"},
                        {FMCKey::KEY3, "laminar/B747/" + fms + "/alphanum_key/3"},
                        {FMCKey::KEY4, "laminar/B747/" + fms + "/alphanum_key/4"},
                        {FMCKey::KEY5, "laminar/B747/" + fms + "/alphanum_key/5"},
                        {FMCKey::KEY6, "laminar/B747/" + fms + "/alphanum_key/6"},
                        {FMCKey::KEY7, "laminar/B747/" + fms + "/alphanum_key/7"},
                        {FMCKey::KEY8, "laminar/B747/" + fms + "/alphanum_key/8"},
                        {FMCKey::KEY9, "laminar/B747/" + fms + "/alphanum_key/9"},
                        {FMCKey::PERIOD, "laminar/B747/" + fms + "/key/period"},
                        {FMCKey::KEY0, "laminar/B747/" + fms + "/alphanum_key/0"},
                        {FMCKey::PLUSMINUS, "laminar/B747/" + fms + "/key/minus"},
                        {FMCKey::KEYA, "laminar/B747/" + fms + "/alphanum_key/A"},
                        {FMCKey::KEYB, "laminar/B747/" + fms + "/alphanum_key/B"},
                        {FMCKey::KEYC, "laminar/B747/" + fms + "/alphanum_key/C"},
                        {FMCKey::KEYD, "laminar/B747/" + fms + "/alphanum_key/D"},
                        {FMCKey::KEYE, "laminar/B747/" + fms + "/alphanum_key/E"},
                        {FMCKey::KEYF, "laminar/B747/" + fms + "/alphanum_key/F"},
                        {FMCKey::KEYG, "laminar/B747/" + fms + "/alphanum_key/G"},
                        {FMCKey::KEYH, "laminar/B747/" + fms + "/alphanum_key/H"},
                        {FMCKey::KEYI, "laminar/B747/" + fms + "/alphanum_key/I"},
                        {FMCKey::KEYJ, "laminar/B747/" + fms + "/alphanum_key/J"},
                        {FMCKey::KEYK, "laminar/B747/" + fms + "/alphanum_key/K"},
                        {FMCKey::KEYL, "laminar/B747/" + fms + "/alphanum_key/L"},
                        {FMCKey::KEYM, "laminar/B747/" + fms + "/alphanum_key/M"},
                        {FMCKey::KEYN, "laminar/B747/" + fms + "/alphanum_key/N"},
                        {FMCKey::KEYO, "laminar/B747/" + fms + "/alphanum_key/O"},
                        {FMCKey::KEYP, "laminar/B747/" + fms + "/alphanum_key/P"},
                        {FMCKey::KEYQ, "laminar/B747/" + fms + "/alphanum_key/Q"},
                        {FMCKey::KEYR, "laminar/B747/" + fms + "/alphanum_key/R"},
                        {FMCKey::KEYS, "laminar/B747/" + fms + "/alphanum_key/S"},
                        {FMCKey::KEYT, "laminar/B747/" + fms + "/alphanum_key/T"},
                        {FMCKey::KEYU, "laminar/B747/" + fms + "/alphanum_key/U"},
                        {FMCKey::KEYV, "laminar/B747/" + fms + "/alphanum_key/V"},
                        {FMCKey::KEYW, "laminar/B747/" + fms + "/alphanum_key/W"},
                        {FMCKey::KEYX, "laminar/B747/" + fms + "/alphanum_key/X"},
                        {FMCKey::KEYY, "laminar/B747/" + fms + "/alphanum_key/Y"},
                        {FMCKey::KEYZ, "laminar/B747/" + fms + "/alphanum_key/Z"},
                        {FMCKey::SPACE, "laminar/B747/" + fms + "/key/space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "laminar/B747/" + fms + "/key/del"},
                        {FMCKey::SLASH, "laminar/B747/" + fms + "/key/slash"},
                        {FMCKey::CLR, "laminar/B747/" + fms + "/key/clear"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &SparkyB744FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &SparkyB744FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'g', FMCTextColor::COLOR_GREEN},
    };
    return colMap;
}

void SparkyB744FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '*':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case 24: // Up arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 25: // Down arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case 26: // Right arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 27: // Left arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case 30: // Up arrow (alternate code)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 31: // Down arrow (alternate code)
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        default:
            // Replace unrecognized characters with spaces to avoid corrupting device output
            if (character < 32 || character > 126) {
                buffer->push_back(' ');
            } else {
                buffer->push_back(character);
            }
            break;
    }
}

void SparkyB744FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto dm = Dataref::getInstance();
    // Display lines: captain=fms3 (observer FMS), FO=fms2, observer=fms1 — B747 quirk
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms3"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "fms1"
                                                                                         : "fms2";
    const std::string base = "laminar/B747/" + fms + "/Line";

    for (int lineNum = 1; lineNum <= (int) ProductFMC::PageLines; ++lineNum) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d", lineNum);
        int lineIndex = lineNum - 1;

        for (bool fontSmall : {false, true}) {
            const std::string ref = base + buf + (fontSmall ? "_S" : "_L");
            std::string text = dm->getCached<std::string>(ref.c_str());
            if (text.empty()) {
                continue;
            }

            // Decode UTF-8 arrow sequences
            size_t pos = 0;
            while ((pos = text.find("\xE2\x86\x91", pos)) != std::string::npos) {
                text.replace(pos, 3, "\x1E"); // Up arrow
            }
            pos = 0;
            while ((pos = text.find("\xE2\x86\x93", pos)) != std::string::npos) {
                text.replace(pos, 3, "\x1F"); // Down arrow
            }
            pos = 0;
            while ((pos = text.find("\xE2\x86\x90", pos)) != std::string::npos) {
                text.replace(pos, 3, "\x1B"); // Left arrow
            }
            pos = 0;
            while ((pos = text.find("\xE2\x86\x92", pos)) != std::string::npos) {
                text.replace(pos, 3, "\x1A"); // Right arrow
            }

            for (int i = 0; i < (int) text.size() && i < (int) ProductFMC::PageCharsPerLine; ++i) {
                unsigned char c = text[i];
                if (c == 0x00) {
                    break;
                }
                if (c != 0x20) {
                    product->writeLineToPage(page, lineIndex, i, std::string(1, toupper(c)), 'g', fontSmall);
                }
            }
        }
    }
}

void SparkyB744FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

#include "stratosphere77w-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

Strato77WFMCProfile::Strato77WFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit2/autopilot/autopilot_has_power", [product](bool powered) {
        uint8_t target = powered ? 200 : 0;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
    }, this);

    // FMC activity (EXEC light) — array: [0]=left, [1]=right, [2]=center
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("Strato/777/cdu_fmc_act", [product](std::vector<float> act) {
        int idx = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? 0
                : product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? 1
                                                                                   : 2;
        bool active = (int)act.size() > idx && act[idx] > 0.5f;
        product->setLedBrightness(FMCLed::PFP_EXEC, active ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, active ? 1 : 0);
    }, this);

}

Strato77WFMCProfile::~Strato77WFMCProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool Strato77WFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("Strato/B777/fms1/Line01_L");
}

const std::vector<std::string> &Strato77WFMCProfile::displayDatarefs() const {
    // Captain display reads come from fms3, observer from fms1 — same quirk as Sparky744
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms3"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "fms1"
                                                                                         : "fms2";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    auto it = cache.find(product->deviceVariant);
    if (it == cache.end()) {
        std::vector<std::string> refs;
        refs.reserve(28);
        for (int i = 1; i <= 14; ++i) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02d", i);
            refs.push_back("Strato/B777/" + fms + "/Line" + buf + "_L");
            refs.push_back("Strato/B777/" + fms + "/Line" + buf + "_S");
        }
        it = cache.emplace(product->deviceVariant, std::move(refs)).first;
    }
    return it->second;
}

const std::vector<FMCButtonDef> &Strato77WFMCProfile::buttonDefs() const {
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms1"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "fms2"
                                                                                             : "fms3";
    const std::string brt = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fmsL"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_FIRSTOFFICER ? "fmsR"
                                                                                             : "fmsC";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "Strato/B777/" + fms + "/ls_key/L1"},
                        {FMCKey::LSK2L, "Strato/B777/" + fms + "/ls_key/L2"},
                        {FMCKey::LSK3L, "Strato/B777/" + fms + "/ls_key/L3"},
                        {FMCKey::LSK4L, "Strato/B777/" + fms + "/ls_key/L4"},
                        {FMCKey::LSK5L, "Strato/B777/" + fms + "/ls_key/L5"},
                        {FMCKey::LSK6L, "Strato/B777/" + fms + "/ls_key/L6"},
                        {FMCKey::LSK1R, "Strato/B777/" + fms + "/ls_key/R1"},
                        {FMCKey::LSK2R, "Strato/B777/" + fms + "/ls_key/R2"},
                        {FMCKey::LSK3R, "Strato/B777/" + fms + "/ls_key/R3"},
                        {FMCKey::LSK4R, "Strato/B777/" + fms + "/ls_key/R4"},
                        {FMCKey::LSK5R, "Strato/B777/" + fms + "/ls_key/R5"},
                        {FMCKey::LSK6R, "Strato/B777/" + fms + "/ls_key/R6"},
                        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_INIT}, "Strato/B777/" + fms + "/func_key/index"},
                        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_SEC_FPLN}, "Strato/B777/" + fms + "/func_key/fpln"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEP_ARR, FMCKey::MCDU_AIRPORT}, "Strato/B777/" + fms + "/func_key/dep_arr"},
                        {std::vector<FMCKey>{FMCKey::PFP7_ALTN, FMCKey::MCDU_ATC_COMM}, "Strato/B777/" + fms + "/func_key/dir_intc"},
                        {std::vector<FMCKey>{FMCKey::PFP7_VNAV, FMCKey::PFP4_VNAV, FMCKey::MCDU_DATA, FMCKey::PFP3_CRZ}, "Strato/B777/" + fms + "/func_key/crz"},
                        {FMCKey::BRIGHTNESS_DOWN, "Strato/777/" + brt + "_brt_dn"},
                        {FMCKey::BRIGHTNESS_UP, "Strato/777/" + brt + "_brt_up"},
                        {std::vector<FMCKey>{FMCKey::PFP_FIX, FMCKey::MCDU_EMPTY_BOTTOM_LEFT}, "Strato/B777/" + fms + "/func_key/fix"},
                        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN, FMCKey::MCDU_DIR}, "Strato/B777/" + fms + "/func_key/legs"},
                        {FMCKey::PFP_HOLD, "Strato/B777/" + fms + "/func_key/hold"},
                        {std::vector<FMCKey>{FMCKey::PFP7_FMC_COMM, FMCKey::PFP4_FMC_COMM}, "Strato/B777/" + fms + "/func_key/index"},
                        {FMCKey::PROG, "Strato/B777/" + fms + "/func_key/prog"},
                        {std::vector<FMCKey>{FMCKey::PFP_EXEC, FMCKey::MCDU_EMPTY_TOP_RIGHT}, "Strato/B777/" + fms + "/key/execute"},
                        {FMCKey::MENU, "Strato/B777/" + fms + "/func_key/index"},
                        {std::vector<FMCKey>{FMCKey::PFP7_NAV_RAD, FMCKey::PFP4_NAV_RAD, FMCKey::MCDU_RAD_NAV}, "Strato/B777/" + fms + "/func_key/navrad"},
                        {FMCKey::PAGE_PREV, "Strato/B777/" + fms + "/func_key/prev_pg"},
                        {FMCKey::PAGE_NEXT, "Strato/B777/" + fms + "/func_key/next_pg"},
                        {FMCKey::KEY1, "Strato/B777/" + fms + "/alphanum_key/1"},
                        {FMCKey::KEY2, "Strato/B777/" + fms + "/alphanum_key/2"},
                        {FMCKey::KEY3, "Strato/B777/" + fms + "/alphanum_key/3"},
                        {FMCKey::KEY4, "Strato/B777/" + fms + "/alphanum_key/4"},
                        {FMCKey::KEY5, "Strato/B777/" + fms + "/alphanum_key/5"},
                        {FMCKey::KEY6, "Strato/B777/" + fms + "/alphanum_key/6"},
                        {FMCKey::KEY7, "Strato/B777/" + fms + "/alphanum_key/7"},
                        {FMCKey::KEY8, "Strato/B777/" + fms + "/alphanum_key/8"},
                        {FMCKey::KEY9, "Strato/B777/" + fms + "/alphanum_key/9"},
                        {FMCKey::PERIOD, "Strato/B777/" + fms + "/key/period"},
                        {FMCKey::KEY0, "Strato/B777/" + fms + "/alphanum_key/0"},
                        {FMCKey::PLUSMINUS, "Strato/B777/" + fms + "/key/minus"},
                        {FMCKey::KEYA, "Strato/B777/" + fms + "/alphanum_key/A"},
                        {FMCKey::KEYB, "Strato/B777/" + fms + "/alphanum_key/B"},
                        {FMCKey::KEYC, "Strato/B777/" + fms + "/alphanum_key/C"},
                        {FMCKey::KEYD, "Strato/B777/" + fms + "/alphanum_key/D"},
                        {FMCKey::KEYE, "Strato/B777/" + fms + "/alphanum_key/E"},
                        {FMCKey::KEYF, "Strato/B777/" + fms + "/alphanum_key/F"},
                        {FMCKey::KEYG, "Strato/B777/" + fms + "/alphanum_key/G"},
                        {FMCKey::KEYH, "Strato/B777/" + fms + "/alphanum_key/H"},
                        {FMCKey::KEYI, "Strato/B777/" + fms + "/alphanum_key/I"},
                        {FMCKey::KEYJ, "Strato/B777/" + fms + "/alphanum_key/J"},
                        {FMCKey::KEYK, "Strato/B777/" + fms + "/alphanum_key/K"},
                        {FMCKey::KEYL, "Strato/B777/" + fms + "/alphanum_key/L"},
                        {FMCKey::KEYM, "Strato/B777/" + fms + "/alphanum_key/M"},
                        {FMCKey::KEYN, "Strato/B777/" + fms + "/alphanum_key/N"},
                        {FMCKey::KEYO, "Strato/B777/" + fms + "/alphanum_key/O"},
                        {FMCKey::KEYP, "Strato/B777/" + fms + "/alphanum_key/P"},
                        {FMCKey::KEYQ, "Strato/B777/" + fms + "/alphanum_key/Q"},
                        {FMCKey::KEYR, "Strato/B777/" + fms + "/alphanum_key/R"},
                        {FMCKey::KEYS, "Strato/B777/" + fms + "/alphanum_key/S"},
                        {FMCKey::KEYT, "Strato/B777/" + fms + "/alphanum_key/T"},
                        {FMCKey::KEYU, "Strato/B777/" + fms + "/alphanum_key/U"},
                        {FMCKey::KEYV, "Strato/B777/" + fms + "/alphanum_key/V"},
                        {FMCKey::KEYW, "Strato/B777/" + fms + "/alphanum_key/W"},
                        {FMCKey::KEYX, "Strato/B777/" + fms + "/alphanum_key/X"},
                        {FMCKey::KEYY, "Strato/B777/" + fms + "/alphanum_key/Y"},
                        {FMCKey::KEYZ, "Strato/B777/" + fms + "/alphanum_key/Z"},
                        {FMCKey::SPACE, "Strato/B777/" + fms + "/key/space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "Strato/B777/" + fms + "/key/del"},
                        {FMCKey::SLASH, "Strato/B777/" + fms + "/key/slash"},
                        {FMCKey::CLR, "Strato/B777/" + fms + "/key/clear"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &Strato77WFMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &Strato77WFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'w', FMCTextColor::COLOR_WHITE},
        {'c', FMCTextColor::COLOR_CYAN},
    };
    return colMap;
}

void Strato77WFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '*':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '`':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case 24:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 25:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case 26:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 27:
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        default:
            if (character < 32 || character > 126) {
                buffer->push_back(' ');
            } else {
                buffer->push_back(character);
            }
            break;
    }
}

void Strato77WFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto dm = Dataref::getInstance();
    // Captain display reads come from fms3, observer from fms1 — same quirk as Sparky744
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN  ? "fms3"
                          : product->deviceVariant == FMCDeviceVariant::VARIANT_OBSERVER ? "fms1"
                                                                                         : "fms2";
    const std::string base = "Strato/B777/" + fms + "/Line";

    for (int lineNum = 1; lineNum <= (int)ProductFMC::PageLines; ++lineNum) {
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
            for (auto [seq, repl] : std::initializer_list<std::pair<const char *, char>>{
                     {"\xE2\x86\x91", '\x18'},
                     {"\xE2\x86\x93", '\x19'},
                     {"\xE2\x86\x90", '\x1B'},
                     {"\xE2\x86\x92", '\x1A'},
                 }) {
                size_t pos = 0;
                while ((pos = text.find(seq, pos)) != std::string::npos) {
                    text.replace(pos, 3, 1, repl);
                }
            }

            // Small font lines use cyan (labels); large font lines use white (data)
            char color = fontSmall ? 'c' : 'w';

            for (int i = 0; i < (int)text.size() && i < (int)ProductFMC::PageCharsPerLine; ++i) {
                unsigned char c = text[i];
                if (c == 0x00) {
                    break;
                }
                if (c != 0x20) {
                    product->writeLineToPage(page, lineIndex, i, std::string(1, toupper(c)), color, fontSmall);
                }
            }
        }
    }
}

void Strato77WFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

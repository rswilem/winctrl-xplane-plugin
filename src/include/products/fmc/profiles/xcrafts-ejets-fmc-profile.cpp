#include "xcrafts-ejets-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

XCraftsEjetsFMCProfile::XCraftsEjetsFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    datarefRegex = std::regex("XCrafts/FMS/CDU_[0-9]+_([0-9]{2}|ScratchPad)");

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontXCrafts);

    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "CDU_1" : "CDU_2";
    Dataref::getInstance()->monitorExistingDataref<int>(("XCrafts/FMS/WW_" + cdu + "_BACKLIGHT").c_str(), [this, product](int brightness) {
        bool powered = Dataref::getInstance()->getCached<bool>("XCrafts/FMS/power_stat");
        product->setLedBrightness(FMCLed::BACKLIGHT, powered ? brightness : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>(("XCrafts/FMS/WW_" + cdu + "_SCREEN_BACKLIGHT").c_str(), [this, product](int brightness) {
        bool powered = Dataref::getInstance()->getCached<bool>("XCrafts/FMS/power_stat");
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, powered ? brightness : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>(("XCrafts/FMS/WW_" + cdu + "_OVERALL_LEDS_BRIGHTNESS").c_str(), [this, product](int brightness) {
        bool powered = Dataref::getInstance()->getCached<bool>("XCrafts/FMS/power_stat");
        product->setLedBrightness(FMCLed::OVERALL_LEDS_BRIGHTNESS, powered ? brightness : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("XCrafts/FMS/power_stat", [this, cdu](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_BACKLIGHT").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_SCREEN_BACKLIGHT").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_OVERALL_LEDS_BRIGHTNESS").c_str());
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>(("XCrafts/FMS/WW_" + cdu + "_EXEC").c_str(), [this, product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_EXEC, (enabled || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_STATUS, (enabled || isAnnunTest()) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>(("XCrafts/FMS/WW_" + cdu + "_CALL").c_str(), [this, product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_CALL_DISPLAY, (enabled || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_MCDU, (enabled || isAnnunTest()) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>(("XCrafts/FMS/WW_" + cdu + "_FAIL").c_str(), [this, product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_FAIL, (enabled || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_FAIL, (enabled || isAnnunTest()) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>(("XCrafts/FMS/WW_" + cdu + "_MSG").c_str(), [this, product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_MSG, (enabled || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_FM, (enabled || isAnnunTest()) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>(("XCrafts/FMS/WW_" + cdu + "_OFST").c_str(), [this, product](bool enabled) {
        product->setLedBrightness(FMCLed::PFP_OFST, (enabled || isAnnunTest()) ? 1 : 0);
        product->setLedBrightness(FMCLed::MCDU_IND, (enabled || isAnnunTest()) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("XCrafts/ERJ/cockpit/annunciators_test", [cdu](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_EXEC").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_CALL").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_FAIL").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_MSG").c_str());
        Dataref::getInstance()->executeChangedCallbacksForDataref(("XCrafts/FMS/WW_" + cdu + "_OFST").c_str());
    }, this);
}

bool XCraftsEjetsFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/FMS/CDU_1_01");
}

const std::vector<std::string> &XCraftsEjetsFMCProfile::displayDatarefs() const {
    const std::string cduNumber = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";
    static std::unordered_map<FMCDeviceVariant, std::vector<std::string>> cache;

    return cache.try_emplace(product->deviceVariant,
                    [cduNumber]() {
                        std::vector<std::string> datarefs;
                        datarefs.push_back("XCrafts/FMS/data_count" + cduNumber);

                        for (int i = 1; i <= 70; i++) {
                            char buffer[32];
                            snprintf(buffer, sizeof(buffer), "XCrafts/FMS/CDU_%s_%02d", cduNumber.c_str(), i);
                            datarefs.push_back(std::string(buffer));
                        }

                        datarefs.push_back("XCrafts/FMS/CDU_" + cduNumber + "_MessagePad");
                        datarefs.push_back("XCrafts/FMS/CDU_" + cduNumber + "_ScratchPad");
                        return datarefs;
                    }())
        .first->second;
}

const std::vector<FMCButtonDef> &XCraftsEjetsFMCProfile::buttonDefs() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "CDU_1" : "CDU_2";
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        // Line Select Keys (XCrafts ERJ format)
                        {FMCKey::LSK1L, "XCrafts/ERJ/" + cdu + "/LSK1"},
                        {FMCKey::LSK2L, "XCrafts/ERJ/" + cdu + "/LSK2"},
                        {FMCKey::LSK3L, "XCrafts/ERJ/" + cdu + "/LSK3"},
                        {FMCKey::LSK4L, "XCrafts/ERJ/" + cdu + "/LSK4"},
                        {FMCKey::LSK5L, "XCrafts/ERJ/" + cdu + "/LSK5"},
                        {FMCKey::LSK6L, "XCrafts/ERJ/" + cdu + "/LSK6"},
                        {FMCKey::LSK1R, "XCrafts/ERJ/" + cdu + "/RSK1"},
                        {FMCKey::LSK2R, "XCrafts/ERJ/" + cdu + "/RSK2"},
                        {FMCKey::LSK3R, "XCrafts/ERJ/" + cdu + "/RSK3"},
                        {FMCKey::LSK4R, "XCrafts/ERJ/" + cdu + "/RSK4"},
                        {FMCKey::LSK5R, "XCrafts/ERJ/" + cdu + "/RSK5"},
                        {FMCKey::LSK6R, "XCrafts/ERJ/" + cdu + "/RSK6"},

                        // {FMCKey::PFP_INIT_REF, "XCrafts/ERJ/" + cdu + "_WW_Key_INIT_REF"},
                        // {FMCKey::PFP_ROUTE, "XCrafts/ERJ/" + cdu + "_WW_Key_RTE"},
                        // {FMCKey::PFP3_CLB, "XCrafts/ERJ/" + cdu + "_WW_Key_CLB"},
                        // {FMCKey::PFP3_CRZ, "XCrafts/ERJ/" + cdu + "_WW_Key_CRZ"},
                        // {FMCKey::PFP3_DES, "XCrafts/ERJ/" + cdu + "_WW_Key_DES"},
                        // {FMCKey::MENU, "XCrafts/ERJ/" + cdu + "_WW_Key_MENU"},
                        // {FMCKey::PFP_LEGS, "XCrafts/ERJ/" + cdu + "_WW_Key_LEGS"},
                        // {FMCKey::PFP_DEP_ARR, "XCrafts/ERJ/" + cdu + "_WW_Key_DEP_ARR"},
                        // {FMCKey::PFP_HOLD, "XCrafts/ERJ/" + cdu + "_WW_Key_HOLD"},
                        // {FMCKey::PROG, "XCrafts/ERJ/" + cdu + "_WW_Key_PROG"},
                        // {FMCKey::PFP_EXEC, "XCrafts/ERJ/" + cdu + "_WW_Key_EXEC"},
                        // {FMCKey::PFP3_N1_LIMIT, "XCrafts/ERJ/" + cdu + "_WW_Key_N1_LIMIT"},
                        // {FMCKey::PFP_FIX, "XCrafts/ERJ/" + cdu + "_WW_Key_FIX"},
                        // {FMCKey::PAGE_PREV, "XCrafts/ERJ/" + cdu + "_WW_Key_PREV_PAGE"},
                        // {FMCKey::PAGE_NEXT, "XCrafts/ERJ/" + cdu + "_WW_Key_NEXT_PAGE"},
                        // {FMCKey::PFP4_ATC, "XCrafts/ERJ/" + cdu + "_WW_Key_ATC"},
                        // {std::vector<FMCKey>{FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, "XCrafts/ERJ/" + cdu + "_WW_Key_VNAV"},
                        // {std::vector<FMCKey>{FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, "XCrafts/ERJ/" + cdu + "_WW_Key_FMC_COMM"},
                        // {std::vector<FMCKey>{FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "XCrafts/ERJ/" + cdu + "_WW_Key_NAV_RAD"},
                        // {FMCKey::PFP7_ALTN, "XCrafts/ERJ/" + cdu + "_WW_Key_ALTN"},

                        {std::vector<FMCKey>{FMCKey::MCDU_SEC_FPLN, FMCKey::PFP_INIT_REF}, "XCrafts/ERJ/" + cdu + "/Key_NAV,XCrafts/ERJ/" + cdu + "/LSK4"}, // Nav IDENT
                        {FMCKey::PFP3_CLB, "XCrafts/ERJ/" + cdu + "/Key_PERF,XCrafts/ERJ/" + cdu + "/RSK2"},                                                // Perf CLIMB
                        {FMCKey::PFP3_CRZ, "XCrafts/ERJ/" + cdu + "/Key_PERF,XCrafts/ERJ/" + cdu + "/LSK2"},                                                // Perf CRUISE
                        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP3_DES}, "XCrafts/ERJ/" + cdu + "/Key_PERF,XCrafts/ERJ/" + cdu + "/LSK3"},     // Perf DESCENT
                                                                                                                                                            //        {std::vector<FMCKey>{FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, "XCrafts/ERJ/"+cdu+"/Key_NAV,XCrafts/ERJ/"+cdu+"/LSK3"}, // Nav TOD details
                                                                                                                                                            //        {FMCKey::PFP_HOLD, "XCrafts/ERJ/"+cdu+"/Key_NAV,XCrafts/ERJ/"+cdu+"/LSK3"}, // Nav HOLD
                                                                                                                                                            //        {FMCKey::PFP_FIX, "XCrafts/ERJ/"+cdu+"/Key_NAV,XCrafts/ERJ/"+cdu+"/LSK3"}, // Nav PILOT WAYPOINT

                        //        {FMCKey::PFP_EXEC, "custom_tbd"},
                        //        {FMCKey::PFP_DEP_ARR, "custom_tbd"},

                        {FMCKey::PROG, "XCrafts/ERJ/" + cdu + "/Key_PROG"},
                        {FMCKey::PFP3_N1_LIMIT, "XCrafts/ERJ/" + cdu + "/Key_TRS"},
                        {FMCKey::MCDU_PERF, "XCrafts/ERJ/" + cdu + "/Key_PERF"},
                        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP_ROUTE}, "XCrafts/ERJ/" + cdu + "/Key_RTE"},
                        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, "XCrafts/ERJ/" + cdu + "/Key_DLK"},
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS}, "XCrafts/ERJ/" + cdu + "/Key_FPL"},
                        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "XCrafts/ERJ/" + cdu + "/Key_RADIO"},
                        {FMCKey::MENU, "XCrafts/ERJ/" + cdu + "/Key_DLK"},
                        {FMCKey::PAGE_PREV, "XCrafts/ERJ/" + cdu + "/Key_PREV"},
                        {FMCKey::PAGE_NEXT, "XCrafts/ERJ/" + cdu + "/Key_NEXT"},

                        // Numeric Keys
                        {FMCKey::KEY1, "XCrafts/ERJ/" + cdu + "/Key_1"},
                        {FMCKey::KEY2, "XCrafts/ERJ/" + cdu + "/Key_2"},
                        {FMCKey::KEY3, "XCrafts/ERJ/" + cdu + "/Key_3"},
                        {FMCKey::KEY4, "XCrafts/ERJ/" + cdu + "/Key_4"},
                        {FMCKey::KEY5, "XCrafts/ERJ/" + cdu + "/Key_5"},
                        {FMCKey::KEY6, "XCrafts/ERJ/" + cdu + "/Key_6"},
                        {FMCKey::KEY7, "XCrafts/ERJ/" + cdu + "/Key_7"},
                        {FMCKey::KEY8, "XCrafts/ERJ/" + cdu + "/Key_8"},
                        {FMCKey::KEY9, "XCrafts/ERJ/" + cdu + "/Key_9"},
                        {FMCKey::KEY0, "XCrafts/ERJ/" + cdu + "/Key_0"},
                        {FMCKey::PERIOD, "XCrafts/ERJ/" + cdu + "/Key_Decimal"},
                        {FMCKey::PLUSMINUS, "XCrafts/ERJ/" + cdu + "/Key_PlusMinus"},

                        // Alpha Keys
                        {FMCKey::KEYA, "XCrafts/ERJ/" + cdu + "/Key_A"},
                        {FMCKey::KEYB, "XCrafts/ERJ/" + cdu + "/Key_B"},
                        {FMCKey::KEYC, "XCrafts/ERJ/" + cdu + "/Key_C"},
                        {FMCKey::KEYD, "XCrafts/ERJ/" + cdu + "/Key_D"},
                        {FMCKey::KEYE, "XCrafts/ERJ/" + cdu + "/Key_E"},
                        {FMCKey::KEYF, "XCrafts/ERJ/" + cdu + "/Key_F"},
                        {FMCKey::KEYG, "XCrafts/ERJ/" + cdu + "/Key_G"},
                        {FMCKey::KEYH, "XCrafts/ERJ/" + cdu + "/Key_H"},
                        {FMCKey::KEYI, "XCrafts/ERJ/" + cdu + "/Key_I"},
                        {FMCKey::KEYJ, "XCrafts/ERJ/" + cdu + "/Key_J"},
                        {FMCKey::KEYK, "XCrafts/ERJ/" + cdu + "/Key_K"},
                        {FMCKey::KEYL, "XCrafts/ERJ/" + cdu + "/Key_L"},
                        {FMCKey::KEYM, "XCrafts/ERJ/" + cdu + "/Key_M"},
                        {FMCKey::KEYN, "XCrafts/ERJ/" + cdu + "/Key_N"},
                        {FMCKey::KEYO, "XCrafts/ERJ/" + cdu + "/Key_O"},
                        {FMCKey::KEYP, "XCrafts/ERJ/" + cdu + "/Key_P"},
                        {FMCKey::KEYQ, "XCrafts/ERJ/" + cdu + "/Key_Q"},
                        {FMCKey::KEYR, "XCrafts/ERJ/" + cdu + "/Key_R"},
                        {FMCKey::KEYS, "XCrafts/ERJ/" + cdu + "/Key_S"},
                        {FMCKey::KEYT, "XCrafts/ERJ/" + cdu + "/Key_T"},
                        {FMCKey::KEYU, "XCrafts/ERJ/" + cdu + "/Key_U"},
                        {FMCKey::KEYV, "XCrafts/ERJ/" + cdu + "/Key_V"},
                        {FMCKey::KEYW, "XCrafts/ERJ/" + cdu + "/Key_W"},
                        {FMCKey::KEYX, "XCrafts/ERJ/" + cdu + "/Key_X"},
                        {FMCKey::KEYY, "XCrafts/ERJ/" + cdu + "/Key_Y"},
                        {FMCKey::KEYZ, "XCrafts/ERJ/" + cdu + "/Key_Z"},

                        // Brightness Controls
                        {FMCKey::BRIGHTNESS_UP, "XCrafts/ERJ/" + cdu + "/Key_BRT"},
                        {FMCKey::BRIGHTNESS_DOWN, "XCrafts/ERJ/" + cdu + "/Key_DIM"},

                        // Special Keys
                        {FMCKey::SPACE, "XCrafts/ERJ/" + cdu + "/Key_Space"},
                        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "XCrafts/ERJ/" + cdu + "/Key_DEL"},
                        {FMCKey::SLASH, "XCrafts/ERJ/" + cdu + "/Key_Slash_Command"},
                        {FMCKey::CLR, "XCrafts/ERJ/" + cdu + "/Key_CLR"}})
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &XCraftsEjetsFMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &XCraftsEjetsFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colors = {
        {0x00, FMCTextColor::COLOR_WHITE},
        {0x01, FMCTextColor::COLOR_CYAN},
        {0x02, FMCTextColor::COLOR_GREEN},
        {0x03, FMCTextColor::COLOR_YELLOW},
        {0x04, FMCTextColor::COLOR_MAGENTA},
        {0x05, FMCTextColor::COLOR_RED},
        {0x06, FMCTextColor::COLOR_LIGHTBROWN},
        {0x07, FMCTextColor::COLOR_AMBER},
        {0x08, FMCTextColor::COLOR_GREY},

        {0xF0, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_WHITE)},
        {0xF1, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_CYAN)},
        {0xF2, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_GREEN)},
        {0xF3, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_YELLOW)},
        {0xF4, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_MAGENTA)},
        {0xF5, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_RED)},
        {0xF6, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_LIGHTBROWN)},
        {0xF7, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_AMBER)},
        {0xF8, FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_BLACK, FMCTextColor::COLOR_GREY)},
    };

    return colors;
}

bool XCraftsEjetsFMCProfile::isAnnunTest() {
    return Dataref::getInstance()->get<bool>("XCrafts/ERJ/cockpit/annunciators_test");
}

void XCraftsEjetsFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case 'a': // THIN ARRW RT - Rightwards arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 'b': // SLD ARRW BI - Black up-pointing triangle
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_UP.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_UP.end());
            break;

        case 'c': // TWIDDLE - @
            buffer->push_back('@');
            break;

        case 'd': // DEGREES - Deg
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case '\\':
        case 'e': // BLOCK - Full block
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_SQUARE.begin(), FMCSpecialCharacter::FILLED_SQUARE.end());
            break;

        case '$':
        case 'g': // SLD ARRW LT - Black left-pointing triangle
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_LEFT.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_LEFT.end());
            break;

        case '?':
        case 'h': // OPEN BLK - White square
            buffer->insert(buffer->end(), FMCSpecialCharacter::WHITE_SQUARE.begin(), FMCSpecialCharacter::WHITE_SQUARE.end());
            break;

        case 'k': // TURN KNOB NOCTR - Refresh right
            buffer->insert(buffer->end(), FMCSpecialCharacter::TRIANGLE.begin(), FMCSpecialCharacter::TRIANGLE.end());
            break;

        case 'm': // THIN ARRW DN - Downwards arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        case 'n': // TRUE DEGREES - Grave accent
            buffer->push_back('`');
            break;

        case 'o': // EMPTY TICK CIRCLE - Ballot box
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case 'p': // GREEN FILLED TICK CIRCLE - White hexagon
            buffer->insert(buffer->end(), FMCSpecialCharacter::DIAMOND.begin(), FMCSpecialCharacter::DIAMOND.end());
            break;

        case 'q': // THIN ARRW LT - Leftwards arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case 'r': // FILLED TICK CIRCLE - Black down-pointing triangle
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_DOWN.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_DOWN.end());
            break;

        case 's': // XPDR DOT - Black square
            buffer->insert(buffer->end(), FMCSpecialCharacter::BLACK_SQUARE.begin(), FMCSpecialCharacter::BLACK_SQUARE.end());
            break;

        case 'u': // THIN ARRW UP - Upwards arrow
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 'v': // SLD ARRW RT - Black right-pointing triangle
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_RIGHT.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_RIGHT.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

void XCraftsEjetsFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    auto datarefManager = Dataref::getInstance();
    const std::string cduNumber = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "1" : "2";

    int dataCount = datarefManager->getCached<int>(("XCrafts/FMS/data_count" + cduNumber).c_str());
    for (int i = 1; i <= std::min(dataCount, 70); i++) {
        char datarefName[32];
        snprintf(datarefName, sizeof(datarefName), "XCrafts/FMS/CDU_%s_%02d", cduNumber.c_str(), i);

        std::vector<unsigned char> text = datarefManager->getCached<std::vector<unsigned char>>(datarefName);

        if (text.empty() || text.size() < 6) {
            continue;
        }

        // Parse XCrafts format: RRCCFS[TEXT]
        // RR CC F S
        // 13 15 6 0 10DEPARTUREv
        // 13 10 6 0 15ACT APP SPEEDv
        // 13 17 2 0 TAKEOFFv
        // 13 01 2 0 $CLEAR            APPLYv
        // 11 01 1 8 $TCAS/XPDR
        // RR = Row (01-99), CC = Column (01-99), F = Font (1-6), S = Color (0-8)

        unsigned short row = (text[0] - '0') * 10 + (text[1] - '0');
        unsigned short col = (text[2] - '0') * 10 + (text[3] - '0');

        XCraftsFMCFontStyle fontStyle = XCraftsFMCFontStyle::Large;
        if (text.size() >= 5) {
            fontStyle = XCraftsFMCFontStyle(text[4] - '0');
        }

        unsigned char colorCode = text[5] - '0';
        if (fontStyle >= XCraftsFMCFontStyle::LargeReverseVideo) {
            colorCode = 0xF0 + colorCode;
        }

        int lineIndex = row - 1;
        int colIndex = col - 1;

        if (lineIndex < 0 || lineIndex >= ProductFMC::PageLines || colIndex < 0) {
            continue;
        }

        unsigned char textStartIndex = 6;
        bool isSmallFont = fontStyle == XCraftsFMCFontStyle::Small || fontStyle == XCraftsFMCFontStyle::SmallReverseVideo || fontStyle == XCraftsFMCFontStyle::SmallReverseVideoBoxed;
        bool isBoxed = fontStyle == XCraftsFMCFontStyle::SmallReverseVideoBoxed || fontStyle == XCraftsFMCFontStyle::LargeReverseVideoBoxed;
        if (isBoxed) {
            if (text.size() > 6 && std::isdigit(text[6])) {
                textStartIndex = 7;
                if (text.size() > 7 && std::isdigit(text[7])) {
                    textStartIndex = 8;
                }
            }
        }

        if (text.size() > textStartIndex) {
            for (int j = textStartIndex; j < text.size() && (colIndex + (j - textStartIndex)) < ProductFMC::PageCharsPerLine; j++) {
                unsigned char c = text[j];
                if (c == 0x00 || c == 0x0A || c == 0x0D) { // Null terminator or newline/carriage return, end of text
                    break;
                }

                int displayCol = colIndex + (j - textStartIndex);
                char existing = product->getPageCharacter(page, lineIndex, displayCol);
                if (!isBoxed && existing && c == 0x20) {
                    continue;
                }

                product->writeLineToPage(page, lineIndex, displayCol, std::string(1, (char) c), colorCode, isSmallFont);
            }
        }
    }

    // First check for MessagePad, which overrides ScratchPad
    std::vector<unsigned char> displayText;
    std::vector<unsigned char> messagePadText = datarefManager->getCached<std::vector<unsigned char>>(("XCrafts/FMS/CDU_" + cduNumber + "_MessagePad").c_str());

    if (!messagePadText.empty() && messagePadText[0] != 0x20) {
        displayText = messagePadText;
    } else {
        // Fall back to ScratchPad
        displayText = datarefManager->getCached<std::vector<unsigned char>>(("XCrafts/FMS/CDU_" + cduNumber + "_ScratchPad").c_str());
    }

    if (!displayText.empty()) {
        for (int i = 0; i < displayText.size() && i < ProductFMC::PageCharsPerLine; ++i) {
            unsigned char c = displayText[i];
            if (c == 0x00 || c == '|') {
                break;
            }
            product->writeLineToPage(page, 13, i, std::string(1, (char) c), 0, false);
        }
    }
}

void XCraftsEjetsFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
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

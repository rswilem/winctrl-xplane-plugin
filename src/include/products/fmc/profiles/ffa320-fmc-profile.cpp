#include "ffa320-fmc-profile.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "ffsharedvalues.h"
#include "logger.hpp"
#include "product-fmc.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

FFA320FMCProfile::FFA320FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    const std::string mcdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "MCDU1" : "MCDU2";

    fmgsPath = "Aircraft.FMGS." + mcdu;
    cockpitPath = "Aircraft.Cockpit." + mcdu;
    commandPrefix = "a320/" + mcdu;
    displayVersionRef = std::string(PRODUCT_NAME "/ffa320/") + mcdu + "/display_version";

    for (unsigned int line = 0; line < FFA320FMCProfile::LineCount; ++line) {
        const std::string suffix = std::to_string(line + 1);
        linePaths[line] = fmgsPath + ".DisplayLines" + suffix;
        attrPaths[line] = fmgsPath + ".DisplayAttrs" + suffix;
    }

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::FontAirbus);

    Dataref::getInstance()->createDataref<int>(displayVersionRef.c_str(), &displayVersion);

    connectToAircraft();
}

FFA320FMCProfile::~FFA320FMCProfile() {
    FFSharedValues::getInstance()->removeUpdateCallback(this);

    if (retained) {
        FFSharedValues::getInstance()->release();
        retained = false;
    }

    // The accessor points at our displayVersion member, so it cannot outlive us.
    Dataref::getInstance()->unbind(displayVersionRef.c_str());
}

bool FFA320FMCProfile::IsEligible() {
    const std::string author = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_author");
    const std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");

    return author.starts_with("FlightFactor") && icao == "A320";
}

void FFA320FMCProfile::connectToAircraft() {
    if (retained) {
        return;
    }

    auto shared = FFSharedValues::getInstance();

    if (!shared->retain(FFSharedValues::A320UltimateSignature)) {
        // The aircraft answers the interface request only once its systems
        // plugin is fully up, which is later than our profile match.
        AppState::getInstance()->executeAfter(2000, this, [this]() {
            connectToAircraft();
        });
        return;
    }

    retained = true;

    shared->addUpdateCallback(this, [this](double step) {
        pollAircraft();
    });
}

void FFA320FMCProfile::pollAircraft() {
    auto shared = FFSharedValues::getInstance();
    if (!shared->isConnected()) {
        return;
    }

    // Names the culprit once if the tree is up but the display object is not
    // where the web MCDU addresses it, which is the one thing that cannot be
    // verified without the aircraft.
    if (!loggedMissingDisplay && shared->idForPath(linePaths[0]) < 0) {
        loggedMissingDisplay = true;
        Logger::getInstance()->warn("FF A320: shared value \"%s\" did not resolve, the MCDU display will stay blank.\n", linePaths[0].c_str());
    }

    bool changed = false;

    for (unsigned int line = 0; line < FFA320FMCProfile::LineCount; ++line) {
        std::string text = shared->getString(linePaths[line]);
        std::string attr = shared->getString(attrPaths[line]);

        if (text != lines[line] || attr != attrs[line]) {
            lines[line] = std::move(text);
            attrs[line] = std::move(attr);
            changed = true;
        }
    }

    if (changed) {
        displayVersion++;
    }

    updateAnnunciators();
    updateBrightness();
}

void FFA320FMCProfile::updateAnnunciators() {
    // The seven lamps on the MCDU bezel, read from the cockpit light objects
    // rather than the raw FMGS signals so cockpit power and dimming are already
    // accounted for.
    static const struct {
            const char *value;
            FMCLed led;
    } annunciators[] = {
        {"AnnuFAIL_Light", FMCLed::MCDU_FAIL},
        {"AnnuFMGC_Light", FMCLed::MCDU_FM},
        {"AnnuMENU_Light", FMCLed::MCDU_MENU},
        {"AnnuSYS1_Light", FMCLed::MCDU_FM1},
        {"AnnuSYS2_Light", FMCLed::MCDU_FM2},
        {"AnnuIND_Light", FMCLed::MCDU_IND},
        {"AnnuRDY_Light", FMCLed::MCDU_RDY},
    };

    auto shared = FFSharedValues::getInstance();

    for (const auto &annunciator : annunciators) {
        double intensity = shared->getNumber(cockpitPath + "." + annunciator.value + ".Intensity");
        product->setLedBrightness(annunciator.led, intensity > 0.001 ? 1 : 0);
    }
}

void FFA320FMCProfile::updateBrightness() {
    auto shared = FFSharedValues::getInstance();

    // The MCDU sits on the pedestal, so its keyboard backlight follows the
    // pedestal integral lighting rather than a unit of its own.
    double integral = std::clamp(shared->getNumber("Aircraft.Cockpit.Pedestal.IntegTarget"), 0.0, 1.0);
    product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(integral * 255));

    bool hasContent = false;
    for (const std::string &line : lines) {
        if (line.find_first_not_of(' ') != std::string::npos) {
            hasContent = true;
            break;
        }
    }

    double brightness = shared->getNumber(fmgsPath + ".Brightness");

    if (!loggedBrightness && hasContent) {
        loggedBrightness = true;
        Logger::getInstance()->debug("FF A320 MCDU brightness reads %f.\n", brightness);
    }

    // The scale of Brightness is not documented anywhere in the aircraft, so it
    // is used as a ratio while it looks like one and falls back to full
    // brightness otherwise; a dark screen would be worse than an undimmed one.
    uint8_t target = (brightness > 0.0 && brightness <= 1.0) ? static_cast<uint8_t>(brightness * 255) : 255;
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, hasContent ? target : 0);
}

const std::vector<std::string> &FFA320FMCProfile::displayDatarefs() const {
    static std::unordered_map<std::string, std::vector<std::string>> cache;

    return cache.try_emplace(displayVersionRef, std::vector<std::string>{displayVersionRef}).first->second;
}

const std::vector<FMCButtonDef> &FFA320FMCProfile::buttonDefs() const {
    static std::unordered_map<FMCDeviceVariant, std::vector<FMCButtonDef>> cache;

    // SET_VALUE carries one or more "leaf=value" assignments into this MCDU's
    // FMGS object, in the order the aircraft's own web MCDU writes them.
    // BRT and DIM are the only keys that go through X-Plane commands, because
    // the aircraft publishes those two as commands and they carry no index.
    return cache.try_emplace(product->deviceVariant,
                    std::vector<FMCButtonDef>{
                        {FMCKey::LSK1L, "PressedDispLine=0;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK2L, "PressedDispLine=1;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK3L, "PressedDispLine=2;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK4L, "PressedDispLine=3;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK5L, "PressedDispLine=4;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK6L, "PressedDispLine=5;PressedDispSide=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK1R, "PressedDispLine=0;PressedDispSide=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK2R, "PressedDispLine=1;PressedDispSide=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK3R, "PressedDispLine=2;PressedDispSide=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK4R, "PressedDispLine=3;PressedDispSide=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK5R, "PressedDispLine=4;PressedDispSide=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::LSK6R, "PressedDispLine=5;PressedDispSide=1", FMCDatarefType::SET_VALUE},

                        {FMCKey::MCDU_DIR, "PressedMenuPage=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::PROG, "PressedMenuPage=1", FMCDatarefType::SET_VALUE},
                        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_N1_LIMIT}, "PressedMenuPage=2", FMCDatarefType::SET_VALUE},
                        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP_INIT_REF}, "PressedMenuPage=3", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_DATA, "PressedMenuPage=4", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_EMPTY_TOP_RIGHT, ""},
                        {FMCKey::BRIGHTNESS_UP, commandPrefix + "/Brt_button", FMCDatarefType::EXECUTE_CMD_ONCE},
                        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS}, "PressedMenuPage=5", FMCDatarefType::SET_VALUE},
                        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "PressedMenuPage=6", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_FUEL_PRED, "PressedMenuPage=7", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_SEC_FPLN, "PressedMenuPage=8", FMCDatarefType::SET_VALUE},
                        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, "PressedMenuPage=9", FMCDatarefType::SET_VALUE},
                        {FMCKey::MENU, "PressedMenuPage=10", FMCDatarefType::SET_VALUE},
                        {FMCKey::BRIGHTNESS_DOWN, commandPrefix + "/Dim_button", FMCDatarefType::EXECUTE_CMD_ONCE},
                        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, "PressedMenuPage=11", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_EMPTY_BOTTOM_LEFT, ""},
                        {FMCKey::PAGE_PREV, "PressedMenuPage=12", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_PAGE_UP, "PressedMenuPage=13", FMCDatarefType::SET_VALUE},
                        {FMCKey::PAGE_NEXT, "PressedMenuPage=14", FMCDatarefType::SET_VALUE},
                        {FMCKey::MCDU_PAGE_DOWN, "PressedMenuPage=15", FMCDatarefType::SET_VALUE},

                        {FMCKey::KEY1, "PressedNum=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY2, "PressedNum=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY3, "PressedNum=2", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY4, "PressedNum=3", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY5, "PressedNum=4", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY6, "PressedNum=5", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY7, "PressedNum=6", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY8, "PressedNum=7", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY9, "PressedNum=8", FMCDatarefType::SET_VALUE},
                        {FMCKey::PERIOD, "PressedNum=9", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEY0, "PressedNum=10", FMCDatarefType::SET_VALUE},
                        {FMCKey::PLUSMINUS, "PressedNum=11", FMCDatarefType::SET_VALUE},

                        {FMCKey::KEYA, "PressedKey=0", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYB, "PressedKey=1", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYC, "PressedKey=2", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYD, "PressedKey=3", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYE, "PressedKey=4", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYF, "PressedKey=5", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYG, "PressedKey=6", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYH, "PressedKey=7", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYI, "PressedKey=8", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYJ, "PressedKey=9", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYK, "PressedKey=10", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYL, "PressedKey=11", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYM, "PressedKey=12", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYN, "PressedKey=13", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYO, "PressedKey=14", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYP, "PressedKey=15", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYQ, "PressedKey=16", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYR, "PressedKey=17", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYS, "PressedKey=18", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYT, "PressedKey=19", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYU, "PressedKey=20", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYV, "PressedKey=21", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYW, "PressedKey=22", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYX, "PressedKey=23", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYY, "PressedKey=24", FMCDatarefType::SET_VALUE},
                        {FMCKey::KEYZ, "PressedKey=25", FMCDatarefType::SET_VALUE},
                        {FMCKey::SLASH, "PressedKey=26", FMCDatarefType::SET_VALUE},
                        {FMCKey::SPACE, "PressedKey=27", FMCDatarefType::SET_VALUE},
                        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_DEL}, "PressedKey=28", FMCDatarefType::SET_VALUE},
                        {FMCKey::CLR, "PressedKey=29", FMCDatarefType::SET_VALUE},
                    })
        .first->second;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FFA320FMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &FFA320FMCProfile::colorMap() const {
    // Attribute letters as the aircraft's own web MCDU decodes them, lowercased
    // because the case only carries the font size.
    static const std::map<char, FMCTextColor> colMap = {
        {'w', FMCTextColor::COLOR_WHITE},
        {'g', FMCTextColor::COLOR_GREEN},
        {'b', FMCTextColor::COLOR_CYAN},
        {'m', FMCTextColor::COLOR_MAGENTA},
        {'y', FMCTextColor::COLOR_YELLOW},
        {'a', FMCTextColor::COLOR_AMBER},
        {'r', FMCTextColor::COLOR_RED},
        {'x', FMCTextColor::COLOR_GREY},
    };

    return colMap;
}

// The FMGS never emits lowercase text, so it reuses b through g for the glyphs
// the character set has no room for. Same substitution table as ConvertChars()
// in the aircraft's data/HTML/JS/mcdu.js.
void FFA320FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case 'b':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_LEFT.begin(), FMCSpecialCharacter::ARROW_LEFT.end());
            break;

        case 'c':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_RIGHT.begin(), FMCSpecialCharacter::ARROW_RIGHT.end());
            break;

        case 'd':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case 'e':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case 'f':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_UP.begin(), FMCSpecialCharacter::ARROW_UP.end());
            break;

        case 'g':
            buffer->insert(buffer->end(), FMCSpecialCharacter::ARROW_DOWN.begin(), FMCSpecialCharacter::ARROW_DOWN.end());
            break;

        default:
            if (character >= 0x80) {
                // The one byte the web MCDU cannot decode either, where it also
                // falls back to a degree sign. Passing it through raw would
                // break the UTF-8 the device expects.
                buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
                break;
            }

            buffer->push_back(character);
            break;
    }
}

void FFA320FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageBytesPerLine, ' '));

    // Reading it here is what puts the ref in the dataref cache, so the product
    // keeps seeing our version bumps as a display change.
    Dataref::getInstance()->getCached<int>(displayVersionRef.c_str());

    for (unsigned int line = 0; line < FFA320FMCProfile::LineCount && line < ProductFMC::PageLines; ++line) {
        const std::string &text = lines[line];
        const std::string &attr = attrs[line];

        for (unsigned int pos = 0; pos < FFA320FMCProfile::CharsPerLine && pos < text.size(); ++pos) {
            char character = text[pos];
            if (character == '\0' || character == ' ') {
                continue;
            }

            char attribute = pos < attr.size() ? attr[pos] : 'W';
            bool fontSmall = attribute >= 'a' && attribute <= 'z';
            char color = fontSmall ? attribute : static_cast<char>(attribute - 'A' + 'a');

            if (colorMap().find(color) == colorMap().end()) {
                color = 'w';
            }

            product->writeLineToPage(page, line, pos, std::string(1, character), color, fontSmall);
        }
    }
}

void FFA320FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    if (button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
        return;
    }

    if (button->datarefType != FMCDatarefType::SET_VALUE) {
        return;
    }

    auto shared = FFSharedValues::getInstance();
    if (!shared->isConnected()) {
        return;
    }

    std::stringstream assignments(button->dataref);
    std::string assignment;

    while (std::getline(assignments, assignment, ';')) {
        size_t separator = assignment.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        shared->setNumber(fmgsPath + "." + assignment.substr(0, separator), std::strtod(assignment.c_str() + separator + 1, nullptr));
    }
}

#include "aerobask-phenom-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>

// Aerobask Phenom 300: its pedestal keyboard is X-Plane's GCU 478, mapped onto the FMC. The G1000
// shows its pages on the MFD, so the FMC screen stays dark and only the keys are used. The FMS
// knob is on the arrow keys: left/right turn the outer ring, up/down the inner one.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";

    const std::string kGcu = "sim/GPS/gcu478/";
} // namespace

AerobaskPhenomFMCProfile::AerobaskPhenomFMCProfile(ProductFMC *product) :
    FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 0);

    // Same lighting as the MCP and PDC
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kPanelBrightness, [this](const std::vector<float> &) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBacklight();
    },
        this);

    // The monitors only report changes, so a dark panel would otherwise keep the product's default brightness
    updateBacklight();
}

bool AerobaskPhenomFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

const std::vector<std::string> &AerobaskPhenomFMCProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::vector<FMCButtonDef> &AerobaskPhenomFMCProfile::buttonDefs() const {
    // Keys are held for as long as the hardware key, so the cockpit key moves with it
    static const std::vector<FMCButtonDef> buttons = {
        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS, FMCKey::PFP_ROUTE}, kGcu + "fpl"},
        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, kGcu + "proc"},
        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_FIX}, kGcu + "direct"},
        {FMCKey::MENU, kGcu + "menu"},
        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_EXEC}, kGcu + "ent"},
        {FMCKey::CLR, kGcu + "clr"},
        {std::vector<FMCKey>{FMCKey::MCDU_EMPTY_BOTTOM_LEFT, FMCKey::PFP_DEL}, kGcu + "bksp"},

        // FMS knob
        {FMCKey::PAGE_PREV, kGcu + "outer_down"},
        {FMCKey::PAGE_NEXT, kGcu + "outer_up"},
        {FMCKey::MCDU_PAGE_DOWN, kGcu + "inner_down"},
        {FMCKey::MCDU_PAGE_UP, kGcu + "inner_up"},
        {FMCKey::MCDU_EMPTY_TOP_RIGHT, kGcu + "cursor"},

        // What the FMS knob tunes
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, kGcu + "nav"},
        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, kGcu + "com"},

        {FMCKey::KEY0, kGcu + "0"},
        {FMCKey::KEY1, kGcu + "1"},
        {FMCKey::KEY2, kGcu + "2"},
        {FMCKey::KEY3, kGcu + "3"},
        {FMCKey::KEY4, kGcu + "4"},
        {FMCKey::KEY5, kGcu + "5"},
        {FMCKey::KEY6, kGcu + "6"},
        {FMCKey::KEY7, kGcu + "7"},
        {FMCKey::KEY8, kGcu + "8"},
        {FMCKey::KEY9, kGcu + "9"},
        {FMCKey::PERIOD, kGcu + "dot"},
        {FMCKey::PLUSMINUS, kGcu + "minus"},
        {FMCKey::SPACE, kGcu + "spc"},

        {FMCKey::KEYA, kGcu + "A"},
        {FMCKey::KEYB, kGcu + "B"},
        {FMCKey::KEYC, kGcu + "C"},
        {FMCKey::KEYD, kGcu + "D"},
        {FMCKey::KEYE, kGcu + "E"},
        {FMCKey::KEYF, kGcu + "F"},
        {FMCKey::KEYG, kGcu + "G"},
        {FMCKey::KEYH, kGcu + "H"},
        {FMCKey::KEYI, kGcu + "I"},
        {FMCKey::KEYJ, kGcu + "J"},
        {FMCKey::KEYK, kGcu + "K"},
        {FMCKey::KEYL, kGcu + "L"},
        {FMCKey::KEYM, kGcu + "M"},
        {FMCKey::KEYN, kGcu + "N"},
        {FMCKey::KEYO, kGcu + "O"},
        {FMCKey::KEYP, kGcu + "P"},
        {FMCKey::KEYQ, kGcu + "Q"},
        {FMCKey::KEYR, kGcu + "R"},
        {FMCKey::KEYS, kGcu + "S"},
        {FMCKey::KEYT, kGcu + "T"},
        {FMCKey::KEYU, kGcu + "U"},
        {FMCKey::KEYV, kGcu + "V"},
        {FMCKey::KEYW, kGcu + "W"},
        {FMCKey::KEYX, kGcu + "X"},
        {FMCKey::KEYY, kGcu + "Y"},
        {FMCKey::KEYZ, kGcu + "Z"},
    };
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &AerobaskPhenomFMCProfile::buttonKeyMap() const {
    static const std::unordered_map<FMCKey, const FMCButtonDef *> map = [this] {
        std::unordered_map<FMCKey, const FMCButtonDef *> keys;
        for (const auto &button : buttonDefs()) {
            std::visit([&](auto &&key) {
                using T = std::decay_t<decltype(key)>;
                if constexpr (std::is_same_v<T, FMCKey>) {
                    keys[key] = &button;
                } else {
                    for (FMCKey k : key) {
                        keys[k] = &button;
                    }
                }
            },
                button.key);
        }
        return keys;
    }();
    return map;
}

const std::map<char, FMCTextColor> &AerobaskPhenomFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colors = {};
    return colors;
}

void AerobaskPhenomFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    buffer->push_back(character);
}

void AerobaskPhenomFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));
}

void AerobaskPhenomFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

void AerobaskPhenomFMCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
}

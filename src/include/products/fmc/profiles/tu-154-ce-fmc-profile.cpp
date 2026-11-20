#include "tu-154-ce-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>

// The Tu-154 has a single GNS 430, so every FMC drives it. The GNS has no keyboard, so the FMC's
// letter and number keys stay unused; its function keys sit on the FMC keys closest in meaning,
// with the large knob on the up and down arrows and the small one on the left and right ones (the
// page keys). The screen stays blank. The GNS can be removed from the cockpit, and then the FMC
// does nothing and stays dark.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    // 1 while the GNS is fitted to the cockpit
    constexpr const char *kShowGns = "tu154b2/custom/anim/show_gns";

    // The keys follow the pedestal lighting like the ABSU panel; it runs from 0 to 1.1
    constexpr const char *kPedestalLighting = "tu154b2/custom/lights/mid_left_panel_int";
    constexpr float kPedestalLightingFull = 1.1f;

    const std::string kGns = "sim/GPS/g430n1_";
} // namespace

Tu154CEFMCProfile::Tu154CEFMCProfile(ProductFMC *product) :
    FMCAircraftProfile(product) {
    product->setAllLedsEnabled(false);
    // Nothing is drawn on the screen, so keep it dark
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 0);

    Dataref::getInstance()->monitorExistingDataref<float>(kPedestalLighting, [this](float) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kShowGns, [this](bool) {
        updateBacklight();
    },
        this);

    // The monitors only report changes, so a dark panel would otherwise keep the product's default brightness
    updateBacklight();
}

bool Tu154CEFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("tu154b2/custom/buttons/console/absu_zk");
}

const std::vector<std::string> &Tu154CEFMCProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::vector<FMCButtonDef> &Tu154CEFMCProfile::buttonDefs() const {
    // Keys are held for as long as the hardware key, so a long CLR press clears the whole entry
    static const std::vector<FMCButtonDef> buttons = {
        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS, FMCKey::PFP_ROUTE}, kGns + "fpl"},
        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, kGns + "proc"},
        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_FIX}, kGns + "direct"},
        {FMCKey::MENU, kGns + "menu"},
        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_EXEC}, kGns + "ent"},
        {FMCKey::CLR, kGns + "clr"},
        {FMCKey::PROG, kGns + "msg"},
        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP_HOLD}, kGns + "obs"},
        {std::vector<FMCKey>{FMCKey::MCDU_INIT, FMCKey::PFP4_VNAV, FMCKey::PFP7_VNAV}, kGns + "vnav"},
        // Switches the CDI between GPS and VLOC
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, kGns + "cdi"},

        // Large knob, small knob and its push
        {FMCKey::MCDU_PAGE_DOWN, kGns + "chapter_dn"},
        {FMCKey::MCDU_PAGE_UP, kGns + "chapter_up"},
        {FMCKey::PAGE_PREV, kGns + "page_dn"},
        {FMCKey::PAGE_NEXT, kGns + "page_up"},
        {FMCKey::MCDU_EMPTY_TOP_RIGHT, kGns + "cursor"},
    };
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &Tu154CEFMCProfile::buttonKeyMap() const {
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

const std::map<char, FMCTextColor> &Tu154CEFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colors = {};
    return colors;
}

void Tu154CEFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    buffer->push_back(character);
}

void Tu154CEFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));
}

void Tu154CEFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue || !hasGns()) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

bool Tu154CEFMCProfile::hasGns() const {
    return Dataref::getInstance()->getCached<bool>(kShowGns);
}

void Tu154CEFMCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn) && hasGns();
    float level = powered ? dataref->getCached<float>(kPedestalLighting) / kPedestalLightingFull : 0.0f;
    product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(level, 0.0f, 1.0f) * 255));
}

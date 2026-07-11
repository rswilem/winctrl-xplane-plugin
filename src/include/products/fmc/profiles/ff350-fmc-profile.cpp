#include "ff350-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cstdint>
#include <variant>

// FF A350 pedestal MCDU brightness knob. Continuous, writable float in [0, 1].
// The in-sim knob, the physical WinWing BRIGHT/DIM keys and the hardware screen
// backlight all track this single value so they stay in sync.
static constexpr const char *kBrightnessDataref = "1-sim/lights/mcdu/Rotery";

// Step applied per BRIGHT/DIM key press. The in-sim knob moves in 0.1 steps.
static constexpr float kBrightnessStep = 0.1f;

FF350FMCProfile::FF350FMCProfile(ProductFMC *product) : TolissFMCProfile(product) {
    auto datarefManager = Dataref::getInstance();

    // ToLiss drives the MCDU screen backlight from AirbusFBW/DUBrightness[6] and
    // flashes it during a self-test keyed to AirbusFBW/DUSelfTestTimeLeft. On the
    // FF A350 that dataref is not the value the pedestal brightness knob moves,
    // and the AirbusFBW self-test datarefs do not represent this panel, so drop
    // both monitors the base constructor registered and rewire the backlight to
    // the knob instead. executeChangedCallbacksForDataref() calls that the
    // inherited power monitors still make for DUBrightness become no-ops once the
    // ref is unbound.
    datarefManager->unbind("AirbusFBW/DUBrightness");
    datarefManager->unbind("AirbusFBW/DUSelfTestTimeLeft");

    auto applyScreenBrightness = [product]() {
        auto datarefManager = Dataref::getInstance();
        bool hasPower = datarefManager->get<bool>("sim/cockpit/electrical/avionics_on");
        bool hasComPower = datarefManager->get<bool>("sim/cockpit2/radios/actuators/com1_power");
        float knob = datarefManager->get<float>(kBrightnessDataref);
        uint8_t screenBrightness = (hasPower && hasComPower) ? static_cast<uint8_t>(knob * 255) : 0;
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, screenBrightness);
    };

    // Re-evaluate the backlight when the knob moves or when bus power toggles, so
    // the panel comes up at the current knob setting on power-up rather than only
    // after the knob is next touched.
    datarefManager->monitorExistingDataref<float>(kBrightnessDataref, [applyScreenBrightness](float) { applyScreenBrightness(); }, this);
    datarefManager->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [applyScreenBrightness](bool) { applyScreenBrightness(); }, this);
    datarefManager->monitorExistingDataref<bool>("sim/cockpit2/radios/actuators/com1_power", [applyScreenBrightness](bool) { applyScreenBrightness(); }, this);
}

bool FF350FMCProfile::IsEligible() {
    auto datarefManager = Dataref::getInstance();
    // AirbusFBW/DUBrightness alone also matches the ToLiss A3xx, so require the
    // FF-specific 1-sim brightness dataref to disambiguate. This branch is checked
    // before TolissFMCProfile in ProductFMC::setProfileForCurrentAircraft().
    return datarefManager->exists("AirbusFBW/DUBrightness") && datarefManager->exists(kBrightnessDataref);
}

void FF350FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    // Retarget the MCDU BRIGHT/DIM keys onto the pedestal brightness knob so the
    // keys, the in-sim knob and the hardware backlight all move the same value.
    // The keys are fully handled here (one step per press, every other phase
    // swallowed) so the inherited ToLiss KeyBright/KeyDim command never fires.
    // Everything else falls through to the ToLiss handling unchanged.
    if (button && std::holds_alternative<FMCKey>(button->key)) {
        FMCKey key = std::get<FMCKey>(button->key);
        if (key == FMCKey::BRIGHTNESS_UP || key == FMCKey::BRIGHTNESS_DOWN) {
            if (phase == xplm_CommandBegin) {
                auto datarefManager = Dataref::getInstance();
                float step = key == FMCKey::BRIGHTNESS_UP ? kBrightnessStep : -kBrightnessStep;
                float next = std::clamp(datarefManager->get<float>(kBrightnessDataref) + step, 0.0f, 1.0f);
                datarefManager->set<float>(kBrightnessDataref, next);
            }
            return;
        }
    }

    TolissFMCProfile::buttonPressed(button, phase);
}

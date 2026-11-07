#include "aerobask-phenom-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

// Aerobask Phenom 300: a PFD and an MFD on X-Plane's G1000. The captain's PDC drives the PFD and
// the first officer's the MFD; both set the pilot's altimeter. The map range works the MFD map
// from both sides, not the PFD inset. The MINS knob is the FMS knob: slow turns the inner ring,
// fast the outer, RST pushes it.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";

    // A held knob starts repeating after this delay
    constexpr float kRepeatDelay = 0.5f;
    constexpr float kSlowRepeatInterval = 0.15f;
    // The slow contact can open again just before the fast one closes, and
    // closes again briefly on the way back from the fast detent
    constexpr float kFastEngageWindow = 0.15f;
    constexpr float kFastReleaseWindow = 0.3f;
} // namespace

AerobaskPhenomPDCProfile::AerobaskPhenomPDCProfile(ProductPDC *product) :
    PDCAircraftProfile(product) {
    // Same lighting as the MCP, so both panels follow one knob
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

bool AerobaskPhenomPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &AerobaskPhenomPDCProfile::buttonDefs() const {
    // try_emplace would build the whole table before the lookup; this runs per button index.
    auto it = buttonCache.find(product->deviceVariant);
    if (it != buttonCache.end()) {
        return it->second;
    }

    const std::string unit = std::string("sim/GPS/") + (product->isCaptainSide() ? "g1000n1_" : "g1000n3_");
    const std::string rangeUnit = "sim/GPS/g1000n3_";
    const std::string rangeCmds = rangeUnit + "range_down," + rangeUnit + "range_up";

    return buttonCache.try_emplace(product->deviceVariant,
                          std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                              // The G1000 sets these through its softkeys, which change meaning from page to page
                              {{0, 0}, {"FPV", ""}},
                              {{1, 1}, {"MTRS", ""}},
                              {{-1, 2}, {"3M VSD", ""}},
                              {{2, 3}, {"WXR", ""}},
                              {{3, 4}, {"STA", ""}},
                              {{4, 5}, {"WPT", ""}},
                              {{5, 6}, {"ARPT", ""}},
                              {{6, 7}, {"DATA", ""}},
                              {{7, 8}, {"POS", ""}},
                              {{8, 9}, {"TERR", ""}},
                              {{9, 10}, {"LEFT VOR1", ""}},
                              {{10, 11}, {"LEFT OFF", ""}},
                              {{11, 12}, {"LEFT ADF1", ""}},
                              {{12, 13}, {"RIGHT VOR2", ""}},
                              {{13, 14}, {"RIGHT OFF", ""}},
                              {{14, 15}, {"RIGHT ADF2", ""}},
                              {{16, 17}, {"VOR MAP CTR", ""}},
                              {{17, 18}, {"RANGE TFC", ""}},
                              {{23, 24}, {"Mins RADIO", ""}},
                              {{24, 25}, {"Mins BARO", ""}},
                              {{27, 28}, {"Map APP", ""}},
                              {{28, 29}, {"Map VOR", ""}},
                              {{29, 30}, {"Map MAP", ""}},
                              {{30, 31}, {"Map PLN", ""}},

                              {{15, 16}, {"Mins RST", unit + "cursor"}},
                              {{19, 32}, {"Mins knob left fast", unit + "fms_outer_down", PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                              {{39, 33}, {"Mins knob left slow", unit + "fms_inner_down", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                              {{40, 34}, {"Mins knob center", ""}},
                              {{41, 35}, {"Mins knob right slow", unit + "fms_inner_up", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                              {{20, 36}, {"Mins knob right fast", unit + "fms_outer_up", PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},

                              // The G1000 has no STD mode: the STD BARO softkey sets 29.92 inHg
                              {{18, 19}, {"Baro STD", "sim/instruments/barometer_std"}},
                              {{25, 26}, {"Baro inHg", "sim/physics/metric_press", PDCDatarefType::SET_VALUE, 0.0}},
                              {{26, 27}, {"Baro HPA", "sim/physics/metric_press", PDCDatarefType::SET_VALUE, 1.0}},
                              {{21, 22}, {"Baro knob left fast", unit + "baro_down", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                              {{42, 37}, {"Baro knob left slow", unit + "baro_down", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                              {{43, 38}, {"Baro knob center", ""}},
                              {{44, 39}, {"Baro knob right slow", unit + "baro_up", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                              {{22, 23}, {"Baro knob right fast", unit + "baro_up", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},

                              {{-1, 20}, {"3M Range Minus", rangeUnit + "range_down"}},
                              {{-1, 21}, {"3M Range Plus", rangeUnit + "range_up"}},
                              // The G1000 range has more steps than the selector and no readable
                              // position, so the selector steps the range knob by the detents turned.
                              {{31, -1}, {"3N Map range 5", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
                              {{32, -1}, {"3N Map range 10", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
                              {{33, -1}, {"3N Map range 20", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
                              {{34, -1}, {"3N Map range 40", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
                              {{35, -1}, {"3N Map range 80", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
                              {{36, -1}, {"3N Map range 160", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
                              {{37, -1}, {"3N Map range 320", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 6.0}},
                              {{38, -1}, {"3N Map range 640", rangeCmds, PDCDatarefType::SET_VALUE_USING_COMMANDS, 7.0}},
                          })
        .first->second;
}

void AerobaskPhenomPDCProfile::update() {
    float now = XPLMGetElapsedTime();

    for (KnobState *knob : {&baroKnob, &fmsKnob}) {
        if (knob->pendingSlow && now >= knob->pendingSlowTime) {
            Dataref::getInstance()->executeCommand(knob->pendingSlow->dataref.c_str());
            knob->pendingSlow = nullptr;
        }

        const PDCButtonDef *held = knob->fast ? knob->fast : knob->slow;
        if (!held || now < knob->lastCommandTime) {
            continue;
        }

        knob->lastCommandTime = now + (knob->fast ? knob->fastRepeatInterval : kSlowRepeatInterval);
        if (!knob->fast) {
            knob->slowFired = true;
        }
        Dataref::getInstance()->executeCommand(held->dataref.c_str());
    }
}

void AerobaskPhenomPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        knobPressed(baroKnob, button, phase);
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        knobPressed(fmsKnob, button, phase);
        return;
    }

    // Everything else acts once per press
    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->datarefType == PDCDatarefType::SET_VALUE_USING_COMMANDS) {
        stepRange(button);
    } else if (button->datarefType == PDCDatarefType::SET_VALUE) {
        Dataref::getInstance()->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void AerobaskPhenomPDCProfile::knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase) {
    bool isFast = std::abs(button->value) > 1.0;
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandBegin) {
        knob.lastCommandTime = now + kRepeatDelay;

        if (isFast) {
            knob.fast = button;
            knob.fastSeen = true;
            knob.pendingSlow = nullptr;
            Dataref::getInstance()->executeCommand(button->dataref.c_str());
        } else {
            bool returningFromFast = now - knob.fastReleaseTime < kFastReleaseWindow;
            knob.slow = button;
            knob.fastSeen = false;
            knob.slowFired = !knob.deferSlow || returningFromFast;
            if (!knob.deferSlow && !returningFromFast) {
                Dataref::getInstance()->executeCommand(button->dataref.c_str());
            }
        }
        return;
    }

    if (isFast) {
        // Back in the slow detent: no single step on release, but repeat again if held
        knob.fast = nullptr;
        knob.fastReleaseTime = now;
        knob.slowFired = true;
        knob.lastCommandTime = now + kRepeatDelay;
    } else {
        if (!knob.slowFired && !knob.fastSeen) {
            knob.pendingSlow = button;
            knob.pendingSlowTime = now + kFastEngageWindow;
        }
        knob.slow = nullptr;
        knob.fastSeen = false;
    }
}

void AerobaskPhenomPDCProfile::stepRange(const PDCButtonDef *button) {
    // Format: "decrease_cmd,increase_cmd"
    size_t comma = button->dataref.find(',');
    if (comma == std::string::npos) {
        return;
    }

    int position = static_cast<int>(button->value);
    if (lastRangePosition >= 0) {
        const std::string command = position > lastRangePosition ? button->dataref.substr(comma + 1) : button->dataref.substr(0, comma);
        for (int step = std::abs(position - lastRangePosition); step > 0; step--) {
            Dataref::getInstance()->executeCommand(command.c_str());
        }
    }

    // The first position reported only sets the reference point
    lastRangePosition = position;
}

void AerobaskPhenomPDCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(PDCLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
}

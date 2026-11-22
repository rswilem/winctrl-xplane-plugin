#include "tu-154-ce-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

// The Tu-154 has no EFIS, so the PDC works the altimeters. The IN/HPA switch picks the one the
// BARO knob sets in hPa and STD sets to 1013: the PDC's own side's VBE (IN) or the UVID (HPA).
// MTRS switches the VBE between metres and feet. The RADIO/BARO switch picks what the MINS knob
// sets: the VBE alert altitude in steps of 100 ft or 100 m (RADIO), which RST clears, or the RV-5
// decision height (BARO). The range knob zooms the GNS 430's map, while the GNS is fitted.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    // The PDC sits on the pedestal like the ABSU panel; its lighting runs from 0 to 1.1
    constexpr const char *kPedestalLighting = "tu154b2/custom/lights/mid_left_panel_int";
    constexpr float kPedestalLightingMax = 1.1f;

    // The decision height is the angle of the RV-5's DH needle, from 0 to 340°
    constexpr float kDecisionHeightMax = 340.0f;

    // The UVID's pressure setting and its knob's range, in hPa
    constexpr const char *kUvidPressure = "tu154b2/custom/gauges/alt/uvid_pressure_knob";
    constexpr float kUvidPressureMin = 787.0f;
    constexpr float kUvidPressureMax = 1075.0f;
    constexpr float kStandardPressure = 1013.0f;

    // A held knob steps again after this delay, and then at these intervals
    constexpr float kRepeatDelay = 0.5f;
    constexpr float kSlowRepeatInterval = 0.15f;
    constexpr float kFastRepeatInterval = 0.05f;
    // The slow contact closes again briefly on the way back from the fast detent
    constexpr float kFastReleaseWindow = 0.3f;

    // Button entries name an action on the selected altimeters, not a dataref
    constexpr const char *kSelectAltimeter = "select altimeter";
    constexpr const char *kSelectMinimums = "select minimums";
    constexpr const char *kStd = "std";
    constexpr const char *kReset = "reset";
    constexpr const char *kMetres = "metres";
    constexpr const char *kKnob = "knob";
    constexpr const char *kRangeSelector = "range selector";

    // 1 while the GNS is fitted to the cockpit
    constexpr const char *kShowGns = "tu154b2/custom/anim/show_gns";
    // The GNS takes zoom in for a larger range
    constexpr const char *kRangeDown = "sim/GPS/g430n1_zoom_out";
    constexpr const char *kRangeUp = "sim/GPS/g430n1_zoom_in";

    bool isFast(const PDCButtonDef *button) {
        return std::abs(button->value) > 1.0;
    }
} // namespace

Tu154CEPDCProfile::Tu154CEPDCProfile(ProductPDC *product) :
    PDCAircraftProfile(product),
    side(product->isCaptainSide() ? "left" : "right") {
    Dataref::getInstance()->monitorExistingDataref<float>(kPedestalLighting, [this](float) {
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

bool Tu154CEPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("tu154b2/custom/buttons/console/absu_zk");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &Tu154CEPDCProfile::buttonDefs() const {
    // The value of a selector entry is its Altimeter or Minimums enumerator
    static const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> buttons = {
        {{25, 26}, {"Baro inHg", kSelectAltimeter, PDCDatarefType::SET_VALUE, static_cast<double>(Altimeter::VBE)}},
        {{26, 27}, {"Baro HPA", kSelectAltimeter, PDCDatarefType::SET_VALUE, static_cast<double>(Altimeter::UVID)}},
        {{23, 24}, {"Mins RADIO", kSelectMinimums, PDCDatarefType::SET_VALUE, static_cast<double>(Minimums::AlertAltitude)}},
        {{24, 25}, {"Mins BARO", kSelectMinimums, PDCDatarefType::SET_VALUE, static_cast<double>(Minimums::DecisionHeight)}},

        {{1, 1}, {"MTRS", kMetres, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
        {{18, 19}, {"Baro STD", kStd, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},
        {{15, 16}, {"Mins RST", kReset, PDCDatarefType::SET_VALUE_MOMENTARY, 1.0}},

        // The knob values give the direction, and the fast detent with ±10
        {{21, 22}, {"Baro knob left fast", kKnob, PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
        {{42, 37}, {"Baro knob left slow", kKnob, PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
        {{44, 39}, {"Baro knob right slow", kKnob, PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
        {{22, 23}, {"Baro knob right fast", kKnob, PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
        {{19, 32}, {"Mins knob left fast", kKnob, PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
        {{39, 33}, {"Mins knob left slow", kKnob, PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
        {{41, 35}, {"Mins knob right slow", kKnob, PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
        {{20, 36}, {"Mins knob right fast", kKnob, PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},

        {{-1, 20}, {"3M Range Minus", kRangeDown}},
        {{-1, 21}, {"3M Range Plus", kRangeUp}},
        // The GNS range has more steps than the selector and no readable position, so the
        // selector zooms by the positions turned
        {{31, -1}, {"3N Map range 5", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 0.0}},
        {{32, -1}, {"3N Map range 10", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 1.0}},
        {{33, -1}, {"3N Map range 20", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 2.0}},
        {{34, -1}, {"3N Map range 40", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 3.0}},
        {{35, -1}, {"3N Map range 80", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 4.0}},
        {{36, -1}, {"3N Map range 160", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 5.0}},
        {{37, -1}, {"3N Map range 320", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 6.0}},
        {{38, -1}, {"3N Map range 640", kRangeSelector, PDCDatarefType::SET_VALUE_USING_COMMANDS, 7.0}},
        // The rest of the PDC has no counterpart: there is no map display to switch
    };
    return buttons;
}

void Tu154CEPDCProfile::update() {
    float now = XPLMGetElapsedTime();

    for (KnobState *knob : {&baroKnob, &minimumsKnob}) {
        const PDCButtonDef *held = knob->fast ? knob->fast : knob->slow;
        if (!held || now < knob->nextStepTime) {
            continue;
        }

        knob->nextStepTime = now + (knob->fast ? kFastRepeatInterval : kSlowRepeatInterval);
        turnKnob(held);
    }
}

void Tu154CEPDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->dataref == kSelectAltimeter) {
        if (phase == xplm_CommandBegin) {
            altimeter = static_cast<Altimeter>(static_cast<int>(button->value));
        }
    } else if (button->dataref == kSelectMinimums) {
        if (phase == xplm_CommandBegin) {
            minimums = static_cast<Minimums>(static_cast<int>(button->value));
        }
    } else if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        knobPressed(baroKnob, button, phase);
    } else if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        knobPressed(minimumsKnob, button, phase);
    } else if (button->datarefType == PDCDatarefType::SET_VALUE_MOMENTARY) {
        push(button, phase);
    } else if (phase == xplm_CommandBegin && button->dataref == kRangeSelector) {
        selectRange(static_cast<int>(button->value));
    } else if (phase == xplm_CommandBegin && hasGns()) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

std::string Tu154CEPDCProfile::vbeCommands() const {
    return side == "left" ? "t154/vbe/vbe_l_" : "t154/vbe/vbe_r_";
}

void Tu154CEPDCProfile::push(const PDCButtonDef *button, XPLMCommandPhase phase) {
    auto dataref = Dataref::getInstance();

    if (phase == xplm_CommandEnd) {
        auto held = heldPushes.find(button);
        if (held != heldPushes.end()) {
            dataref->set<int>(held->second.c_str(), 0);
            heldPushes.erase(held);
        }
        return;
    }

    if (button->dataref == kStd && altimeter == Altimeter::UVID) {
        dataref->set<float>(kUvidPressure, kStandardPressure);
        return;
    }

    std::string pushed;
    if (button->dataref == kMetres) {
        pushed = "tu154b2/custom/gauges/alt/vbe_mode_but_" + side;
    } else if (button->dataref == kStd) {
        pushed = "sim/custom/gauges/alt/vbe_" + side + "_p_push";
    } else if (button->dataref == kReset && minimums == Minimums::AlertAltitude) {
        // The RV-5 has no reset
        pushed = "sim/custom/gauges/alt/vbe_" + side + "_h_push";
    } else {
        return;
    }

    dataref->set<int>(pushed.c_str(), static_cast<int>(button->value));
    heldPushes[button] = pushed;
}

void Tu154CEPDCProfile::knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase) {
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandBegin) {
        if (isFast(button)) {
            knob.fast = button;
            knob.nextStepTime = now + kFastRepeatInterval;
            turnKnob(button);
        } else {
            knob.slow = button;
            knob.nextStepTime = now + kRepeatDelay;
            // Skip the step while the fast detent is engaged or being released through
            if (!knob.fast && now - knob.fastReleaseTime > kFastReleaseWindow) {
                turnKnob(button);
            }
        }
        return;
    }

    if (isFast(button)) {
        knob.fast = nullptr;
        knob.fastReleaseTime = now;
        knob.nextStepTime = now + kRepeatDelay;
    } else {
        knob.slow = nullptr;
    }
}

void Tu154CEPDCProfile::turnKnob(const PDCButtonDef *button) {
    auto dataref = Dataref::getInstance();
    bool up = button->value > 0;

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING && altimeter == Altimeter::VBE) {
        // One hPa per command
        dataref->executeCommand((vbeCommands() + (up ? "p_plus" : "p_minus")).c_str());
    } else if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        // One hPa per step
        float pressure = dataref->get<float>(kUvidPressure) + (up ? 1.0f : -1.0f);
        dataref->set<float>(kUvidPressure, std::clamp(pressure, kUvidPressureMin, kUvidPressureMax));
    } else if (minimums == Minimums::AlertAltitude) {
        // 100 ft or m of alert altitude per command
        dataref->executeCommand((vbeCommands() + (up ? "h_plus" : "h_minus")).c_str());
    } else {
        // One degree of the DH needle per step
        std::string decisionHeight = "tu154b2/custom/gauges/alt/radioalt_dh_" + side;
        float angle = dataref->get<float>(decisionHeight.c_str()) + (up ? 1.0f : -1.0f);
        dataref->set<float>(decisionHeight.c_str(), std::clamp(angle, 0.0f, kDecisionHeightMax));
    }
}

void Tu154CEPDCProfile::selectRange(int position) {
    // The first position reported only sets the reference point. Without the GNS the position is
    // still followed, so fitting it again doesn't replay the turns made meanwhile.
    if (lastRangePosition >= 0 && hasGns()) {
        const char *command = position > lastRangePosition ? kRangeUp : kRangeDown;
        for (int step = std::abs(position - lastRangePosition); step > 0; step--) {
            Dataref::getInstance()->executeCommand(command);
        }
    }
    lastRangePosition = position;
}

bool Tu154CEPDCProfile::hasGns() const {
    return Dataref::getInstance()->get<int>(kShowGns) == 1;
}

void Tu154CEPDCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = std::clamp(dataref->getCached<float>(kPedestalLighting) / kPedestalLightingMax, 0.0f, 1.0f);
    product->setLedBrightness(PDCLed::BACKLIGHT, powered ? static_cast<uint8_t>(level * 255) : 0);
}

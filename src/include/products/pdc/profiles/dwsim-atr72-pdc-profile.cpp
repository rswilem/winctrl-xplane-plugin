#include "dwsim-atr72-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <XPLMProcessing.h>

// DeltaWing Simulations ATR 72-600. Each PDC works its own side's EFCP and ICP: the captain's
// EFCP-L and left ICP, the first officer's EFCP-R and right ICP.
// - BARO sets the ICP baro, STD pushes the BARO knob. The PFD shows hPa and inHg together, so the
//   IN/HPA switch has no counterpart.
// - MINS sets the ICP DH / MDA value, and the RADIO/BARO switch its DH (radio) / MDA (baro)
//   selector. The ICP has no reset.
// - The range selector or the 3M's RANGE keys set the ND range, which runs from 5 to 160 NM, so
//   the 3N's 320 and 640 NM positions give 160 NM.
// - The map mode selector turns the FORMAT knob (ARC, ROSE, PLAN), and the VOR/ADF switches the
//   BRG1 / BRG2 keys (VOR, ADF, no pointer). The aircraft doesn't publish the format or the
//   bearing sources, so these turn and press by the positions the switch moved.
// - The MFD keys go to the closest PDC keys: CTR shows the ND, ARPT the airport moving map and
//   DATA the PERF page.
// - FPV, MTRS, WXR, STA, WPT, POS, TERR, TFC and RST have no EFCP or ICP counterpart.

namespace {
    // Main instrument panel and pedestal lighting, 0 to 1
    constexpr const char *kPanelBrightness = "DWSim/ATR_Series/lit/cockpit/mipPed/brightness";

    // A held knob steps again after this delay, and then at these intervals
    constexpr float kRepeatDelay = 0.5f;
    constexpr float kSlowRepeatInterval = 0.15f;
    constexpr float kFastRepeatInterval = 0.05f;
    // The slow contact closes again briefly on the way back from the fast detent
    constexpr float kFastReleaseWindow = 0.3f;

    // X-Plane's map range follows the ND range: 1 = 5 NM to 6 = 160 NM
    constexpr int kRangeMin = 1;
    constexpr int kRangeMax = 6;
    // How long the map range may take to follow the RANGE keys
    constexpr float kRangeSettleTime = 0.5f;

    // The DH / MDA selector: minimums/<side>/ref is 0 for DH, 1 for MDA
    constexpr int kReferenceDh = 0;
    constexpr int kReferenceMda = 1;

    // The BRG keys cycle VOR, ADF, no pointer; the entries' values are positions in that cycle
    constexpr int kBearingVor = 0;
    constexpr int kBearingAdf = 1;
    constexpr int kBearingOff = 2;
    constexpr int kBearingPositions = 3;

    // Button entries name an action, not a dataref
    constexpr const char *kRangeSelector = "range selector";
    constexpr const char *kFormatSelector = "format selector";
    constexpr const char *kMinimumsSelector = "minimums selector";

    bool isFast(const PDCButtonDef *button) {
        return std::abs(button->value) > 1.0;
    }
} // namespace

DWSimATR72PDCProfile::DWSimATR72PDCProfile(ProductPDC *product) :
    PDCAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>(kPanelBrightness, [this](float) {
        updateBacklight();
    },
        this);

    for (const char *power : {"DWSim/ATR_Series/afcs/icp_on_capt", "DWSim/ATR_Series/afcs/icp_on_fo"}) {
        Dataref::getInstance()->monitorExistingDataref<bool>(power, [this](bool) {
            updateBacklight();
        },
            this);
    }

    // The monitors only report changes, so a dark panel would otherwise keep the product's default brightness
    updateBacklight();
}

bool DWSimATR72PDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("DWSim/ATR_Series/afcs/alt_sel_ft");
}

std::string DWSimATR72PDCProfile::efcp() const {
    return product->isCaptainSide() ? "DWSim/ATR_Series/commands/efcpl/" : "DWSim/ATR_Series/commands/efcpr/";
}

std::string DWSimATR72PDCProfile::icp() const {
    return product->isCaptainSide() ? "DWSim/ATR_Series/commands/front/icp/left/" : "DWSim/ATR_Series/commands/front/icp/right/";
}

std::string DWSimATR72PDCProfile::minimums() const {
    return product->isCaptainSide() ? "DWSim/ATR_Series/minimums/captain/" : "DWSim/ATR_Series/minimums/fo/";
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &DWSimATR72PDCProfile::buttonDefs() const {
    // try_emplace would build the whole table before the lookup; this runs per button index.
    auto it = buttonCache.find(product->deviceVariant);
    if (it != buttonCache.end()) {
        return it->second;
    }

    const std::string efcp = this->efcp();
    const std::string icp = this->icp();

    return buttonCache.try_emplace(product->deviceVariant,
                          std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                              {{16, 17}, {"VOR MAP CTR", efcp + "mfd/nd_toggle"}},
                              {{5, 6}, {"ARPT", efcp + "mfd/map_toggle"}},
                              {{6, 7}, {"DATA", efcp + "mfd/perf_toggle"}},

                              {{9, 10}, {"LEFT VOR1", efcp + "nav/brg1_toggle", PDCDatarefType::SET_VALUE, kBearingVor}},
                              {{10, 11}, {"LEFT OFF", efcp + "nav/brg1_toggle", PDCDatarefType::SET_VALUE, kBearingOff}},
                              {{11, 12}, {"LEFT ADF1", efcp + "nav/brg1_toggle", PDCDatarefType::SET_VALUE, kBearingAdf}},
                              {{12, 13}, {"RIGHT VOR2", efcp + "nav/brg2_toggle", PDCDatarefType::SET_VALUE, kBearingVor}},
                              {{13, 14}, {"RIGHT OFF", efcp + "nav/brg2_toggle", PDCDatarefType::SET_VALUE, kBearingOff}},
                              {{14, 15}, {"RIGHT ADF2", efcp + "nav/brg2_toggle", PDCDatarefType::SET_VALUE, kBearingAdf}},

                              {{27, 28}, {"Map APP", kFormatSelector, PDCDatarefType::SET_VALUE, 0.0}},
                              {{28, 29}, {"Map VOR", kFormatSelector, PDCDatarefType::SET_VALUE, 1.0}},
                              {{29, 30}, {"Map MAP", kFormatSelector, PDCDatarefType::SET_VALUE, 2.0}},
                              {{30, 31}, {"Map PLN", kFormatSelector, PDCDatarefType::SET_VALUE, 3.0}},

                              {{23, 24}, {"Mins RADIO", kMinimumsSelector, PDCDatarefType::SET_VALUE, kReferenceDh}},
                              {{24, 25}, {"Mins BARO", kMinimumsSelector, PDCDatarefType::SET_VALUE, kReferenceMda}},
                              {{19, 32}, {"Mins knob left fast", icp + "innerRotationKnob/rotateLeft", PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                              {{39, 33}, {"Mins knob left slow", icp + "innerRotationKnob/rotateLeft", PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                              {{41, 35}, {"Mins knob right slow", icp + "innerRotationKnob/rotateRight", PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                              {{20, 36}, {"Mins knob right fast", icp + "innerRotationKnob/rotateRight", PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},

                              {{18, 19}, {"Baro STD", icp + "baroSetRotationKnob/push"}},
                              {{21, 22}, {"Baro knob left fast", icp + "baroSetRotationKnob/rotateLeft", PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                              {{42, 37}, {"Baro knob left slow", icp + "baroSetRotationKnob/rotateLeft", PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                              {{44, 39}, {"Baro knob right slow", icp + "baroSetRotationKnob/rotateRight", PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                              {{22, 23}, {"Baro knob right fast", icp + "baroSetRotationKnob/rotateRight", PDCDatarefType::ADD_BARO_REPEATING, 10.0}},

                              {{-1, 20}, {"3M Range Minus", efcp + "nav/minus_toggle"}},
                              {{-1, 21}, {"3M Range Plus", efcp + "nav/plus_toggle"}},
                              {{31, -1}, {"3N Map range 5", kRangeSelector, PDCDatarefType::SET_VALUE, 1.0}},
                              {{32, -1}, {"3N Map range 10", kRangeSelector, PDCDatarefType::SET_VALUE, 2.0}},
                              {{33, -1}, {"3N Map range 20", kRangeSelector, PDCDatarefType::SET_VALUE, 3.0}},
                              {{34, -1}, {"3N Map range 40", kRangeSelector, PDCDatarefType::SET_VALUE, 4.0}},
                              {{35, -1}, {"3N Map range 80", kRangeSelector, PDCDatarefType::SET_VALUE, 5.0}},
                              {{36, -1}, {"3N Map range 160", kRangeSelector, PDCDatarefType::SET_VALUE, 6.0}},
                              {{37, -1}, {"3N Map range 320", kRangeSelector, PDCDatarefType::SET_VALUE, 6.0}},
                              {{38, -1}, {"3N Map range 640", kRangeSelector, PDCDatarefType::SET_VALUE, 6.0}},
                              // FPV (0), MTRS (1), WXR (2), STA (3), WPT (4), POS (7), TERR (8), RST (15),
                              // TFC (17) and IN/HPA (25/26) have no counterpart
                          })
        .first->second;
}

void DWSimATR72PDCProfile::update() {
    float now = XPLMGetElapsedTime();

    for (KnobState *knob : {&baroKnob, &minimumsKnob}) {
        const PDCButtonDef *held = knob->fast ? knob->fast : knob->slow;
        if (!held || now < knob->nextStepTime) {
            continue;
        }

        knob->nextStepTime = now + (knob->fast ? kFastRepeatInterval : kSlowRepeatInterval);
        Dataref::getInstance()->executeCommand(held->dataref.c_str());
    }
}

void DWSimATR72PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        knobPressed(baroKnob, button, phase);
        return;
    }

    if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        knobPressed(minimumsKnob, button, phase);
        return;
    }

    // Everything else acts once per press or selector position
    if (phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == kRangeSelector) {
        selectRange(static_cast<int>(button->value));
    } else if (button->dataref == kFormatSelector) {
        selectFormat(static_cast<int>(button->value));
    } else if (button->dataref == kMinimumsSelector) {
        selectMinimums(static_cast<int>(button->value));
    } else if (button->datarefType == PDCDatarefType::SET_VALUE) {
        selectBearing(button);
    } else {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void DWSimATR72PDCProfile::knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase) {
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandBegin) {
        if (isFast(button)) {
            knob.fast = button;
            knob.nextStepTime = now + kFastRepeatInterval;
            Dataref::getInstance()->executeCommand(button->dataref.c_str());
        } else {
            knob.slow = button;
            knob.nextStepTime = now + kRepeatDelay;
            // Skip the step while the fast detent is engaged or being released through
            if (!knob.fast && now - knob.fastReleaseTime > kFastReleaseWindow) {
                Dataref::getInstance()->executeCommand(button->dataref.c_str());
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

void DWSimATR72PDCProfile::selectRange(int range) {
    // The EFCP RANGE keys step the range, which X-Plane's map range follows
    const char *rangeRef = product->isCaptainSide() ? "sim/cockpit2/EFIS/map_range" : "sim/cockpit2/EFIS/map_range_copilot";
    int current = Dataref::getInstance()->get<int>(rangeRef);
    int target = std::clamp(range, kRangeMin, kRangeMax);

    // The map range can lag the keys, so a selector turned on before it caught up counts from the
    // range last asked for
    float now = XPLMGetElapsedTime();
    if (requestedRange >= 0 && now - requestedRangeTime < kRangeSettleTime) {
        current = requestedRange;
    }
    requestedRange = target;
    requestedRangeTime = now;

    const std::string command = efcp() + (target > current ? "nav/plus_toggle" : "nav/minus_toggle");
    for (int step = std::abs(target - current); step > 0; step--) {
        Dataref::getInstance()->executeCommand(command.c_str());
    }
}

void DWSimATR72PDCProfile::selectFormat(int position) {
    // The FORMAT knob turns without stops and its position isn't published, so it turns a click per
    // position the selector moved. The first position reported only sets the reference point.
    if (lastFormatPosition >= 0) {
        const std::string command = efcp() + (position > lastFormatPosition ? "nav/formatKnob/right" : "nav/formatKnob/left");
        for (int step = std::abs(position - lastFormatPosition); step > 0; step--) {
            Dataref::getInstance()->executeCommand(command.c_str());
        }
    }
    lastFormatPosition = position;
}

void DWSimATR72PDCProfile::selectMinimums(int reference) {
    // The outer DH / MDA knob turns towards DH to the left and MDA to the right
    std::string referenceRef = minimums() + "ref";
    if (Dataref::getInstance()->get<int>(referenceRef.c_str()) == reference) {
        return;
    }

    std::string command = icp() + (reference == kReferenceMda ? "outerRotationKnob/rotateRight" : "outerRotationKnob/rotateLeft");
    Dataref::getInstance()->executeCommand(command.c_str());
}

void DWSimATR72PDCProfile::selectBearing(const PDCButtonDef *button) {
    // The BRG key cycles VOR, ADF and no pointer, and its source isn't published, so it is pressed
    // as often as it takes to follow the switch around that cycle. The first position reported
    // only sets the reference point.
    int &last = button->dataref.ends_with("brg1_toggle") ? lastBearing1 : lastBearing2;
    int position = static_cast<int>(button->value);

    if (last >= 0) {
        for (int press = (position - last + kBearingPositions) % kBearingPositions; press > 0; press--) {
            Dataref::getInstance()->executeCommand(button->dataref.c_str());
        }
    }
    last = position;
}

void DWSimATR72PDCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(product->isCaptainSide() ? "DWSim/ATR_Series/afcs/icp_on_capt" : "DWSim/ATR_Series/afcs/icp_on_fo");

    float level = dataref->getCached<float>(kPanelBrightness);
    level = powered && std::isfinite(level) ? std::clamp(level, 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(PDCLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
}

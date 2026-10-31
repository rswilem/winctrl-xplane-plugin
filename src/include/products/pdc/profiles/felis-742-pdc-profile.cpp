#include "felis-742-pdc-profile.h"

#include "dataref.h"
#include "product-pdc.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <XPLMProcessing.h>

// The 747 classic has no EFIS control panel. The PDC drives the ND panel of
// the Felis FMC (shared by both sides), the side's altimeter baro and its
// radio altimeter DH.

namespace {
    constexpr const char *kNDButtons = "B742/FMC/ND_buttons";
    constexpr const char *kNDMode = "B742/FMC/ND_mode";
    constexpr const char *kNDRange = "B742/FMC/ND_range";

    // A held baro knob starts repeating after this delay
    constexpr float kRepeatDelay = 0.5f;
    constexpr float kSlowRepeatInterval = 0.15f;
    constexpr float kFastRepeatInterval = 0.03f;
    // The slow contact closes again briefly on the way back from the fast detent
    constexpr float kFastReleaseWindow = 0.3f;
} // namespace

Felis742PDCProfile::Felis742PDCProfile(ProductPDC *product) :
    PDCAircraftProfile(product) {
    // Same lighting as the MCP, so both panels follow one knob
    auto setBacklight = [product](float brightness) {
        product->setLedBrightness(PDCLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    };
    Dataref::getInstance()->monitorExistingDataref<float>("B742/int_light/AP_int_light", setBacklight, this);

    // The monitor only reports changes, so a dark panel would otherwise keep the product's default brightness
    setBacklight(Dataref::getInstance()->get<float>("B742/int_light/AP_int_light"));
}

bool Felis742PDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("B742/controls/park_brake_lever");
}

const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &Felis742PDCProfile::buttonDefs() const {
    static std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> cache;

    // try_emplace would build the whole table before the lookup; this runs per button index.
    auto it = cache.find(product->deviceVariant);
    if (it != cache.end()) {
        return it->second;
    }

    const bool isCaptain = product->isCaptainSide();
    const std::string baroUp = isCaptain ? "sim/instruments/barometer_up" : "sim/instruments/barometer_copilot_up";
    const std::string baroDown = isCaptain ? "sim/instruments/barometer_down" : "sim/instruments/barometer_copilot_down";
    const std::string baroStd = isCaptain ? "sim/instruments/barometer_std" : "sim/instruments/barometer_copilot_std";
    const std::string dhUp = isCaptain ? "B742/command/DH_left_UP" : "B742/command/DH_right_UP";
    const std::string dhDown = isCaptain ? "B742/command/DH_left_DOWN" : "B742/command/DH_right_DOWN";

    // ND buttons: SET_VALUE_MOMENTARY with the button's index in ND_buttons as the value
    return cache.try_emplace(product->deviceVariant,
                    std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>{
                        {{0, 0}, {"FPV", ""}},
                        {{1, 1}, {"MTRS", ""}},
                        {{-1, 2}, {"3M VSD", ""}},
                        {{2, 3}, {"WXR", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Weather)}},
                        {{3, 4}, {"STA", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Navaid)}},
                        {{4, 5}, {"WPT", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Waypoint)}},
                        {{5, 6}, {"ARPT", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Airport)}},
                        {{6, 7}, {"DATA", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Data)}},
                        {{7, 8}, {"POS", kNDButtons, PDCDatarefType::SET_VALUE_MOMENTARY, static_cast<double>(NDButton::Heading)}},
                        {{8, 9}, {"TERR", ""}},
                        {{9, 10}, {"LEFT VOR1", ""}},
                        {{10, 11}, {"LEFT OFF", ""}},
                        {{11, 12}, {"LEFT ADF1", ""}},
                        {{12, 13}, {"RIGHT VOR2", ""}},
                        {{13, 14}, {"RIGHT OFF", ""}},
                        {{14, 15}, {"RIGHT ADF2", ""}},
                        {{15, 16}, {"Mins RST", ""}},
                        {{16, 17}, {"VOR MAP CTR", ""}},
                        {{17, 18}, {"RANGE TFC", ""}},
                        {{18, 19}, {"Baro STD", baroStd}},
                        {{-1, 20}, {"3M Range Minus", kNDRange, PDCDatarefType::ADD_RANGE_REPEATING, -1.0}},
                        {{-1, 21}, {"3M Range Plus", kNDRange, PDCDatarefType::ADD_RANGE_REPEATING, 1.0}},
                        {{21, 22}, {"Baro knob left fast", baroDown, PDCDatarefType::ADD_BARO_REPEATING, -10.0}},
                        {{22, 23}, {"Baro knob right fast", baroUp, PDCDatarefType::ADD_BARO_REPEATING, 10.0}},
                        {{23, 24}, {"Mins RADIO", ""}},
                        {{24, 25}, {"Mins BARO", ""}},
                        {{25, 26}, {"Baro inHg", ""}},
                        {{26, 27}, {"Baro HPA", ""}},
                        // The ND has no fourth mode, so APP and VOR both select INS
                        {{27, 28}, {"Map APP", kNDMode, PDCDatarefType::SET_VALUE, static_cast<double>(NDMode::INS)}},
                        {{28, 29}, {"Map VOR", kNDMode, PDCDatarefType::SET_VALUE, static_cast<double>(NDMode::INS)}},
                        {{29, 30}, {"Map MAP", kNDMode, PDCDatarefType::SET_VALUE, static_cast<double>(NDMode::Map)}},
                        {{30, 31}, {"Map PLN", kNDMode, PDCDatarefType::SET_VALUE, static_cast<double>(NDMode::Plan)}},
                        // The ND range runs 10-640, so 5 clamps to 10
                        {{31, -1}, {"3N Map range 5", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm10)}},
                        {{32, -1}, {"3N Map range 10", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm10)}},
                        {{33, -1}, {"3N Map range 20", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm20)}},
                        {{34, -1}, {"3N Map range 40", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm40)}},
                        {{35, -1}, {"3N Map range 80", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm80)}},
                        {{36, -1}, {"3N Map range 160", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm160)}},
                        {{37, -1}, {"3N Map range 320", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm320)}},
                        {{38, -1}, {"3N Map range 640", kNDRange, PDCDatarefType::SET_VALUE, static_cast<double>(NDRange::Nm640)}},
                        {{19, 32}, {"Mins knob left fast", dhDown, PDCDatarefType::ADD_MINIMUMS_REPEATING, -10.0}},
                        {{39, 33}, {"Mins knob left slow", dhDown, PDCDatarefType::ADD_MINIMUMS_REPEATING, -1.0}},
                        {{40, 34}, {"Mins knob center", ""}},
                        {{41, 35}, {"Mins knob right slow", dhUp, PDCDatarefType::ADD_MINIMUMS_REPEATING, 1.0}},
                        {{20, 36}, {"Mins knob right fast", dhUp, PDCDatarefType::ADD_MINIMUMS_REPEATING, 10.0}},
                        {{42, 37}, {"Baro knob left slow", baroDown, PDCDatarefType::ADD_BARO_REPEATING, -1.0}},
                        {{43, 38}, {"Baro knob center", ""}},
                        {{44, 39}, {"Baro knob right slow", baroUp, PDCDatarefType::ADD_BARO_REPEATING, 1.0}},
                    })
        .first->second;
}

void Felis742PDCProfile::update() {
    const PDCButtonDef *held = baroKnob.fast ? baroKnob.fast : baroKnob.slow;
    float now = XPLMGetElapsedTime();
    if (!held || now < baroKnob.lastCommandTime) {
        return;
    }

    baroKnob.lastCommandTime = now + (baroKnob.fast ? kFastRepeatInterval : kSlowRepeatInterval);
    Dataref::getInstance()->executeCommand(held->dataref.c_str());
}

void Felis742PDCProfile::buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->datarefType == PDCDatarefType::ADD_BARO_REPEATING) {
        baroKnobPressed(button, phase);
    } else if (button->datarefType == PDCDatarefType::ADD_MINIMUMS_REPEATING) {
        // Here the mins knob holds the DH command instead of adding to a value
        minsKnobPressed(button, phase);
    } else if (button->datarefType == PDCDatarefType::SET_VALUE_MOMENTARY) {
        // Here the value is the index of the button in the ND_buttons array
        pressNDButton(static_cast<NDButton>(static_cast<int>(button->value)), phase == xplm_CommandBegin);
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::ADD_RANGE_REPEATING) {
        // Here the range steps once per press: the 3M range encoder sends one press per detent
        stepNDRange(static_cast<int>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::SET_VALUE) {
        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else if (phase == xplm_CommandBegin && button->datarefType == PDCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    }
}

void Felis742PDCProfile::baroKnobPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    bool isFast = std::abs(button->value) > 1.0;
    float now = XPLMGetElapsedTime();

    if (phase == xplm_CommandEnd) {
        if (isFast) {
            baroKnob.fast = nullptr;
            baroKnob.fastReleaseTime = now;
            baroKnob.lastCommandTime = now + kRepeatDelay;
        } else {
            baroKnob.slow = nullptr;
        }
        return;
    }

    baroKnob.lastCommandTime = now + kRepeatDelay;
    if (isFast) {
        baroKnob.fast = button;
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
        return;
    }

    baroKnob.slow = button;
    // Skip the single step when passing back through the slow position
    if (now - baroKnob.fastReleaseTime >= kFastReleaseWindow) {
        Dataref::getInstance()->executeCommand(button->dataref.c_str());
    }
}

void Felis742PDCProfile::minsKnobPressed(const PDCButtonDef *button, XPLMCommandPhase phase) {
    // The Felis DH knob turns for as long as its command is held, so hold it
    // while the knob is out of the centre position
    if (phase == xplm_CommandBegin) {
        if (minsKnobPositionsHeld++ == 0) {
            minsCommand = button->dataref;
            Dataref::getInstance()->executeCommand(minsCommand.c_str(), xplm_CommandBegin);
        }
    } else if (minsKnobPositionsHeld > 0 && --minsKnobPositionsHeld == 0) {
        Dataref::getInstance()->executeCommand(minsCommand.c_str(), xplm_CommandEnd);
    }
}

void Felis742PDCProfile::pressNDButton(NDButton button, bool pressed) {
    auto datarefManager = Dataref::getInstance();
    std::vector<int> buttons = datarefManager->get<std::vector<int>>(kNDButtons);
    size_t index = static_cast<size_t>(button);
    if (index >= buttons.size()) {
        return;
    }

    buttons[index] = pressed ? 1 : 0;
    datarefManager->set<std::vector<int>>(kNDButtons, buttons);
}

void Felis742PDCProfile::stepNDRange(int delta) {
    auto datarefManager = Dataref::getInstance();
    int range = std::clamp(datarefManager->get<int>(kNDRange) + delta, static_cast<int>(NDRange::Nm10), static_cast<int>(NDRange::Nm640));
    datarefManager->set<int>(kNDRange, range);
}

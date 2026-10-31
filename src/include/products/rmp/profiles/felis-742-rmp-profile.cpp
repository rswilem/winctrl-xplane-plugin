#include "felis-742-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

#include <algorithm>
#include <cstdio>

// The 747 classic VHF control panels have a left and a right frequency window,
// each with its own knobs, and a transfer switch selecting the active one. The
// RMP shows the active window and the other one as standby; its knobs tune the
// standby window and its swap button throws the transfer switch. VHF1-3 select
// which of the three panels this RMP drives.
//
// The NAV panels have a single window, so with ILS (NAV1) or VOR (NAV2)
// selected the standby window stays blank, the knobs tune the active
// frequency and the swap button does nothing.
//
// The ADF panels work like the VHF ones, but each window has three knobs
// (100, 10 and 1 kHz). The large knob tunes 100 kHz and the small knob 10 kHz,
// or 1 kHz after pushing it. The ADF button selects the side's own receiver.

namespace {
    // Button entries name an action on the selected radio, not a dataref
    constexpr const char *kSelectRadio = "select";
    constexpr const char *kSelectADF = "select_adf";
    constexpr const char *kTransfer = "transfer";
    constexpr const char *kKnobPush = "push";
    constexpr const char *kTurnKnob = "knob";

    constexpr const char *kBlank = "      ";

    std::string formatComFrequency(int khz) {
        if (khz <= 0) {
            return kBlank;
        }

        char buf[16];
        snprintf(buf, sizeof(buf), "%.3f", khz / 1000.0);
        return std::string(buf);
    }

    // NAV frequencies are in units of 10 kHz
    std::string formatNavFrequency(int frequency) {
        if (frequency <= 0) {
            return kBlank;
        }

        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", frequency / 100.0);
        return std::string(buf);
    }

    // ADF frequencies are whole kHz, right-aligned in the window
    std::string formatADFFrequency(int khz) {
        if (khz <= 0) {
            return kBlank;
        }

        char buf[16];
        snprintf(buf, sizeof(buf), "%6d", khz);
        return std::string(buf);
    }
} // namespace

Felis742RMPProfile::Felis742RMPProfile(ProductRMP *product) :
    RMPAircraftProfile(product) {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            selectedRadio = Radio::VHF2;
            break;
        case RMPDeviceVariant::VARIANT_STBY:
            selectedRadio = Radio::VHF3;
            break;
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            selectedRadio = Radio::VHF1;
            break;
    }

    product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 255);

    // The VHF panels sit on the pedestal
    auto setBacklight = [product](float brightness) {
        product->setLedBrightness(RMPLed::BACKLIGHT, static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255));
    };
    Dataref::getInstance()->monitorExistingDataref<float>("B742/int_light/pedestal_int_light", setBacklight, this);
    setBacklight(Dataref::getInstance()->get<float>("B742/int_light/pedestal_int_light"));

    auto setPowered = [this, product](bool powered) {
        product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, powered ? 255 : 0);
        updateLeds();
        updateDisplays();
    };
    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", setPowered, this);
    setPowered(Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on"));
}

bool Felis742RMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("B742/controls/park_brake_lever");
}

const std::vector<std::string> &Felis742RMPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/avionics_on",
        "sim/cockpit2/radios/actuators/com1_power",
        "sim/cockpit2/radios/actuators/com2_power",
        "B742/INT_nav/COM3_power",
        "sim/cockpit2/radios/actuators/com1_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com1_standby_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_standby_frequency_hz_833",
        "B742/INT_nav/COM3_freq_active",
        "B742/INT_nav/COM3_freq_left",
        "B742/INT_nav/COM3_freq_right",
        "sim/cockpit2/radios/actuators/nav1_power",
        "sim/cockpit2/radios/actuators/nav2_power",
        "sim/cockpit2/radios/actuators/nav1_frequency_hz",
        "sim/cockpit2/radios/actuators/nav2_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_power",
        "sim/cockpit2/radios/actuators/adf2_power",
        "sim/cockpit2/radios/actuators/adf1_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_standby_frequency_hz",
        "sim/cockpit2/radios/actuators/adf2_frequency_hz",
        "sim/cockpit2/radios/actuators/adf2_standby_frequency_hz",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &Felis742RMPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, RMPButtonDef> buttons = {
        {0, {"Flip frequencies", kTransfer}},
        // The value of a SET_VALUE entry is its Radio or Knob enumerator
        {1, {"VHF 1", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF1)}},
        {2, {"VHF 2", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF2)}},
        {3, {"VHF 3", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF3)}},
        {4, {"LOAD", ""}},
        {5, {"HF1", ""}},
        {6, {"HF2", ""}},
        {7, {"AM", ""}},
        {8, {"NAV", ""}},
        {9, {"VOR", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV2)}},
        {10, {"ILS", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV1)}},
        {11, {"GLS", ""}},
        {12, {"MLS", ""}},
        {13, {"ADF", kSelectADF}},
        {14, {"Knob Large L", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::LargeDown)}},
        {15, {"Knob Large R", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::LargeUp)}},
        {16, {"Knob Small L", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::SmallDown)}},
        {17, {"Knob Small R", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::SmallUp)}},
        {18, {"Knob depress", kKnobPush}},
        {19, {"Switch On", ""}},
        {20, {"Switch Off", ""}},
    };
    return buttons;
}

void Felis742RMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == kSelectRadio) {
        selectRadio(static_cast<Radio>(static_cast<int>(button->value)));
    } else if (button->dataref == kSelectADF) {
        selectRadio(product->deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER ? Radio::ADF2 : Radio::ADF1);
    } else if (button->dataref == kKnobPush && isADFRadio()) {
        adfFineTuning = !adfFineTuning;
    } else if (button->dataref == kTransfer) {
        throwTransferSwitch();
    } else if (button->dataref == kTurnKnob) {
        turnKnob(static_cast<Knob>(static_cast<int>(button->value)));
    }
}

void Felis742RMPProfile::selectRadio(Radio radio) {
    selectedRadio = radio;
    adfFineTuning = false;
    updateLeds();
    updateDisplays();
}

void Felis742RMPProfile::throwTransferSwitch() {
    // The NAV panels have a single window and no transfer switch
    if (isNavRadio()) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    datarefManager->set<int>(transferSwitch().c_str(), isLeftWindowActive() ? 1 : 0);
}

std::string Felis742RMPProfile::transferSwitch() const {
    std::string panel = isADFRadio() ? "B742/ADF" : "B742/COM";
    return panel + std::to_string(radioNumber()) + "/TFR_sw";
}

void Felis742RMPProfile::updateDisplays() {
    auto datarefManager = Dataref::getInstance();
    if (!datarefManager->getCached<bool>("sim/cockpit/electrical/avionics_on") || !isRadioPowered()) {
        product->setDisplayText(kBlank, kBlank);
        return;
    }

    if (isNavRadio()) {
        std::string frequency = "sim/cockpit2/radios/actuators/nav" + std::to_string(radioNumber()) + "_frequency_hz";
        product->setDisplayText(formatNavFrequency(datarefManager->getCached<int>(frequency.c_str())), kBlank);
        return;
    }

    if (isADFRadio()) {
        std::string prefix = "sim/cockpit2/radios/actuators/adf" + std::to_string(radioNumber());
        product->setDisplayText(formatADFFrequency(datarefManager->getCached<int>((prefix + "_frequency_hz").c_str())),
            formatADFFrequency(datarefManager->getCached<int>((prefix + "_standby_frequency_hz").c_str())));
        return;
    }

    int active = 0;
    int standby = 0;
    if (selectedRadio == Radio::VHF3) {
        int left = datarefManager->getCached<int>("B742/INT_nav/COM3_freq_left");
        int right = datarefManager->getCached<int>("B742/INT_nav/COM3_freq_right");
        bool rightActive = datarefManager->getCached<int>("B742/INT_nav/COM3_freq_active") == 1;
        active = rightActive ? right : left;
        standby = rightActive ? left : right;
    } else {
        std::string prefix = "sim/cockpit2/radios/actuators/com" + std::to_string(radioNumber());
        active = datarefManager->getCached<int>((prefix + "_frequency_hz_833").c_str());
        standby = datarefManager->getCached<int>((prefix + "_standby_frequency_hz_833").c_str());
    }

    product->setDisplayText(formatComFrequency(active), formatComFrequency(standby));
}

bool Felis742RMPProfile::isNavRadio() const {
    return selectedRadio == Radio::NAV1 || selectedRadio == Radio::NAV2;
}

bool Felis742RMPProfile::isADFRadio() const {
    return selectedRadio == Radio::ADF1 || selectedRadio == Radio::ADF2;
}

int Felis742RMPProfile::radioNumber() const {
    switch (selectedRadio) {
        case Radio::VHF2:
        case Radio::NAV2:
        case Radio::ADF2:
            return 2;
        case Radio::VHF3:
            return 3;
        default:
            return 1;
    }
}

bool Felis742RMPProfile::isLeftWindowActive() const {
    // The panels' left_active and right_active datarefs are lamp brightnesses that
    // go dark with the panel lighting, so read the switch itself
    return Dataref::getInstance()->get<int>(transferSwitch().c_str()) == 0;
}

bool Felis742RMPProfile::isRadioPowered() const {
    auto datarefManager = Dataref::getInstance();
    if (selectedRadio == Radio::VHF3) {
        return datarefManager->getCached<int>("B742/INT_nav/COM3_power") != 0;
    }

    std::string type = "com";
    if (isNavRadio()) {
        type = "nav";
    } else if (isADFRadio()) {
        type = "adf";
    }

    std::string power = "sim/cockpit2/radios/actuators/" + type + std::to_string(radioNumber()) + "_power";
    return datarefManager->getCached<int>(power.c_str()) != 0;
}

void Felis742RMPProfile::turnKnob(Knob knob) {
    bool largeKnob = knob == Knob::LargeDown || knob == Knob::LargeUp;
    std::string direction = (knob == Knob::LargeUp || knob == Knob::SmallUp) ? "CW" : "CCW";
    std::string radio = std::to_string(radioNumber());

    std::string command;
    if (isNavRadio()) {
        // The NAV panel's only window
        command = "B742/command/NAV" + radio + "_freq_" + (largeKnob ? "big" : "small") + "_" + direction;
    } else {
        // Tune the standby window, the one the transfer switch has not selected
        std::string window = isLeftWindowActive() ? "R" : "L";
        if (isADFRadio()) {
            std::string adfKnob = largeKnob ? "big" : (adfFineTuning ? "small" : "mid");
            command = "B742/command/ADF" + radio + "_" + adfKnob + "_" + window + "_" + direction;
        } else {
            command = "B742/command/VHF" + radio + "_" + (largeKnob ? "big" : "small") + "_" + window + "_" + direction;
        }
    }

    Dataref::getInstance()->executeCommand(command.c_str());
}

void Felis742RMPProfile::updateLeds() {
    bool powered = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on");
    product->setLedBrightness(RMPLed::VHF1, powered && selectedRadio == Radio::VHF1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VHF2, powered && selectedRadio == Radio::VHF2 ? 255 : 0);
    product->setLedBrightness(RMPLed::VHF3, powered && selectedRadio == Radio::VHF3 ? 255 : 0);
    product->setLedBrightness(RMPLed::ILS, powered && selectedRadio == Radio::NAV1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VOR, powered && selectedRadio == Radio::NAV2 ? 255 : 0);
    product->setLedBrightness(RMPLed::ADF, powered && isADFRadio() ? 255 : 0);
}

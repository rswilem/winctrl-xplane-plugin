#include "aerobask-phenom-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

#include <algorithm>
#include <cstdio>

// Aerobask Phenom 300: its G1000 radios are X-Plane's own. VHF1 and VHF2 select COM1 and COM2,
// ILS and VOR select NAV1 and NAV2, and ADF the ADF. The RMP shows the selected radio's active and
// standby frequencies; its knobs tune the standby frequency and its swap button flips the two.
// The G1000 follows every change. The backlight follows the panel lighting, like the MCP and PDC.

namespace {
    // Button entries name an action on the selected radio, not a dataref
    constexpr const char *kSelectRadio = "select";
    constexpr const char *kTransfer = "transfer";
    constexpr const char *kKnobPush = "push";
    constexpr const char *kTurnKnob = "knob";

    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";

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

AerobaskPhenomRMPProfile::AerobaskPhenomRMPProfile(ProductRMP *product) :
    RMPAircraftProfile(product) {
    // The first officer's RMP starts on COM2, the captain's and the standby one on COM1
    selectedRadio = product->deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER ? Radio::COM2 : Radio::COM1;

    product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 255);

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kPanelBrightness, [this](const std::vector<float> &) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBacklight();
        updateLeds();
        updateDisplays();
    },
        this);

    // The monitors only report changes, so a dark panel would otherwise keep the product's defaults
    updateBacklight();
    updateLeds();
}

bool AerobaskPhenomRMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

const std::vector<std::string> &AerobaskPhenomRMPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kAvionicsOn,
        "sim/cockpit2/radios/actuators/com1_power",
        "sim/cockpit2/radios/actuators/com2_power",
        "sim/cockpit2/radios/actuators/com1_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com1_standby_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_standby_frequency_hz_833",
        "sim/cockpit2/radios/actuators/nav1_power",
        "sim/cockpit2/radios/actuators/nav2_power",
        "sim/cockpit2/radios/actuators/nav1_frequency_hz",
        "sim/cockpit2/radios/actuators/nav1_standby_frequency_hz",
        "sim/cockpit2/radios/actuators/nav2_frequency_hz",
        "sim/cockpit2/radios/actuators/nav2_standby_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_power",
        "sim/cockpit2/radios/actuators/adf1_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_standby_frequency_hz",
    };
    return datarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &AerobaskPhenomRMPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, RMPButtonDef> buttons = {
        {0, {"Flip frequencies", kTransfer}},
        // The value of a SET_VALUE entry is its Radio or Knob enumerator
        {1, {"VHF 1", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::COM1)}},
        {2, {"VHF 2", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::COM2)}},
        {3, {"VHF 3", ""}},
        {4, {"LOAD", ""}},
        {5, {"HF1", ""}},
        {6, {"HF2", ""}},
        {7, {"AM", ""}},
        {8, {"NAV", ""}},
        {9, {"VOR", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV2)}},
        {10, {"ILS", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV1)}},
        {11, {"GLS", ""}},
        {12, {"MLS", ""}},
        {13, {"ADF", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::ADF)}},
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

void AerobaskPhenomRMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == kSelectRadio) {
        selectRadio(static_cast<Radio>(static_cast<int>(button->value)));
    } else if (button->dataref == kKnobPush && selectedRadio == Radio::ADF) {
        adfFineTuning = !adfFineTuning;
    } else if (button->dataref == kTransfer) {
        std::string command = "sim/radios/" + radioName() + "_standy_flip";
        Dataref::getInstance()->executeCommand(command.c_str());
    } else if (button->dataref == kTurnKnob) {
        turnKnob(static_cast<Knob>(static_cast<int>(button->value)));
    }
}

void AerobaskPhenomRMPProfile::selectRadio(Radio radio) {
    selectedRadio = radio;
    adfFineTuning = false;
    updateLeds();
    updateDisplays();
}

void AerobaskPhenomRMPProfile::turnKnob(Knob knob) {
    bool largeKnob = knob == Knob::LargeDown || knob == Knob::LargeUp;
    std::string direction = (knob == Knob::LargeUp || knob == Knob::SmallUp) ? "up" : "down";

    // The large knob tunes the MHz (ADF: 100 kHz) and the small one the kHz (ADF: 10 kHz or 1 kHz).
    // The COM radios tune in 8.33 kHz channels.
    std::string step;
    switch (selectedRadio) {
        case Radio::COM1:
        case Radio::COM2:
            step = std::string(largeKnob ? "coarse_" : "fine_") + direction + "_833";
            break;
        case Radio::NAV1:
        case Radio::NAV2:
            step = std::string(largeKnob ? "coarse_" : "fine_") + direction;
            break;
        case Radio::ADF:
            step = std::string(largeKnob ? "hundreds_" : (adfFineTuning ? "ones_" : "tens_")) + direction;
            break;
    }

    std::string command = "sim/radios/stby_" + radioName() + "_" + step;
    Dataref::getInstance()->executeCommand(command.c_str());
}

void AerobaskPhenomRMPProfile::updateDisplays() {
    auto dataref = Dataref::getInstance();
    if (!dataref->getCached<bool>(kAvionicsOn) || !isRadioPowered()) {
        product->setDisplayText(kBlank, kBlank);
        return;
    }

    std::string prefix = "sim/cockpit2/radios/actuators/" + radioName();
    switch (selectedRadio) {
        case Radio::COM1:
        case Radio::COM2:
            product->setDisplayText(formatComFrequency(dataref->getCached<int>((prefix + "_frequency_hz_833").c_str())),
                formatComFrequency(dataref->getCached<int>((prefix + "_standby_frequency_hz_833").c_str())));
            break;
        case Radio::NAV1:
        case Radio::NAV2:
            product->setDisplayText(formatNavFrequency(dataref->getCached<int>((prefix + "_frequency_hz").c_str())),
                formatNavFrequency(dataref->getCached<int>((prefix + "_standby_frequency_hz").c_str())));
            break;
        case Radio::ADF:
            product->setDisplayText(formatADFFrequency(dataref->getCached<int>((prefix + "_frequency_hz").c_str())),
                formatADFFrequency(dataref->getCached<int>((prefix + "_standby_frequency_hz").c_str())));
            break;
    }
}

std::string AerobaskPhenomRMPProfile::radioName() const {
    switch (selectedRadio) {
        case Radio::COM1:
            return "com1";
        case Radio::COM2:
            return "com2";
        case Radio::NAV1:
            return "nav1";
        case Radio::NAV2:
            return "nav2";
        case Radio::ADF:
            return "adf1";
    }
    return "com1";
}

bool AerobaskPhenomRMPProfile::isRadioPowered() const {
    std::string power = "sim/cockpit2/radios/actuators/" + radioName() + "_power";
    return Dataref::getInstance()->getCached<int>(power.c_str()) != 0;
}

void AerobaskPhenomRMPProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(RMPLed::BACKLIGHT, static_cast<uint8_t>(level * 255));
    product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, powered ? 255 : 0);
}

void AerobaskPhenomRMPProfile::updateLeds() {
    bool powered = Dataref::getInstance()->getCached<bool>(kAvionicsOn);
    product->setLedBrightness(RMPLed::VHF1, powered && selectedRadio == Radio::COM1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VHF2, powered && selectedRadio == Radio::COM2 ? 255 : 0);
    product->setLedBrightness(RMPLed::ILS, powered && selectedRadio == Radio::NAV1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VOR, powered && selectedRadio == Radio::NAV2 ? 255 : 0);
    product->setLedBrightness(RMPLed::ADF, powered && selectedRadio == Radio::ADF ? 255 : 0);
}

#include "tu-154-ce-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

#include <algorithm>
#include <array>
#include <cstdio>

// The Tu-154 radios on an RMP. VHF1, VHF2 and VHF3 select the three VHF sets, ILS and VOR select
// NAV1 and NAV2. The sets have no standby frequency, so the knobs tune the active one and the
// standby window stays blank. The VHF sets are tuned through the aircraft's own commands, which
// also turn their knobs in the cockpit.

namespace {
    // Button entries name an action on the selected radio, not a dataref
    constexpr const char *kSelectRadio = "select";
    constexpr const char *kTurnKnob = "knob";

    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    // The RMP sits on the pedestal like the ABSU panel; its lighting runs from 0 to 1.1
    constexpr const char *kPedestalLighting = "tu154b2/custom/lights/mid_left_panel_int";
    constexpr float kPedestalLightingMax = 1.1f;

    constexpr const char *kVhf1Frequency = "sim/cockpit2/radios/actuators/com1_frequency_hz_833";
    constexpr const char *kVhf2Frequency = "sim/cockpit2/radios/actuators/com2_frequency_hz_833";
    // VHF3 is the aircraft's own and has no X-Plane radio
    constexpr const char *kVhf3Mhz = "sim/custom/radios/vhfnew_3_mhz";
    constexpr const char *kVhf3Khz = "sim/custom/radios/vhfnew_3_khz";
    constexpr const char *kNav1Frequency = "sim/cockpit2/radios/actuators/nav1_frequency_hz";
    constexpr const char *kNav2Frequency = "sim/cockpit2/radios/actuators/nav2_frequency_hz";

    // The VHF sets run on the 27 V buses through their overhead switches, as the aircraft powers them.
    // VHF1 and VHF2 power X-Plane's COM radios, but both sets write COM2's power, so it can't be read.
    constexpr const char *kVhf1Switch = "tu154b2/custom/switchers/ovhd/vhf_1_on";
    constexpr const char *kVhf2Switch = "tu154b2/custom/switchers/ovhd/vhf_2_on";
    constexpr const char *kVhf3Switch = "sim/custom/switchers/ovhd/vhf_3_on";
    constexpr const char *kLeftBus = "tu154b2/custom/elec/bus27_volt_left";
    constexpr const char *kRightBus = "tu154b2/custom/elec/bus27_volt_right";
    constexpr float kVhfMinimumVolts = 13.0f;
    // The Kurs-MP sets power X-Plane's NAV radios from their own switches
    constexpr const char *kNav1Power = "sim/cockpit2/radios/actuators/nav1_power";
    constexpr const char *kNav2Power = "sim/cockpit2/radios/actuators/nav2_power";

    constexpr const char *kBlank = "      ";

    // The tuning commands of a radio, in the order of the Knob enumerators
    using TuningCommands = std::array<const char *, 4>;
} // namespace

Tu154CERMPProfile::Tu154CERMPProfile(ProductRMP *product) :
    RMPAircraftProfile(product) {
    selectedRadio = product->deviceVariant == RMPDeviceVariant::VARIANT_FIRSTOFFICER ? Radio::VHF2 : Radio::VHF1;

    product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, 255);

    Dataref::getInstance()->monitorExistingDataref<float>(kPedestalLighting, [this](float) {
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

bool Tu154CERMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("tu154b2/custom/buttons/console/absu_zk");
}

const std::vector<std::string> &Tu154CERMPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        kAvionicsOn,
        kVhf1Frequency,
        kVhf2Frequency,
        kVhf3Mhz,
        kVhf3Khz,
        kNav1Frequency,
        kNav2Frequency,
        kVhf1Switch,
        kVhf2Switch,
        kVhf3Switch,
        kLeftBus,
        kRightBus,
        kNav1Power,
        kNav2Power,
    };
    return datarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &Tu154CERMPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, RMPButtonDef> buttons = {
        // The value of a SET_VALUE entry is its Radio or Knob enumerator
        {1, {"VHF 1", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF1)}},
        {2, {"VHF 2", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF2)}},
        {3, {"VHF 3", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::VHF3)}},
        {9, {"VOR", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV2)}},
        {10, {"ILS", kSelectRadio, RMPDatarefType::SET_VALUE, static_cast<double>(Radio::NAV1)}},
        {14, {"Knob Large L", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::LargeDown)}},
        {15, {"Knob Large R", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::LargeUp)}},
        {16, {"Knob Small L", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::SmallDown)}},
        {17, {"Knob Small R", kTurnKnob, RMPDatarefType::SET_VALUE, static_cast<double>(Knob::SmallUp)}},
        // The swap button, the other radios and the power switch have no counterpart
    };
    return buttons;
}

void Tu154CERMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase != xplm_CommandBegin) {
        return;
    }

    if (button->dataref == kSelectRadio) {
        selectRadio(static_cast<Radio>(static_cast<int>(button->value)));
    } else if (button->dataref == kTurnKnob) {
        turnKnob(static_cast<Knob>(static_cast<int>(button->value)));
    }
}

void Tu154CERMPProfile::selectRadio(Radio radio) {
    selectedRadio = radio;
    updateLeds();
    updateDisplays();
}

void Tu154CERMPProfile::turnKnob(Knob knob) {
    // The large knob tunes the MHz and the small one the kHz
    TuningCommands commands;
    switch (selectedRadio) {
        case Radio::VHF1:
            commands = {"t154/vhf/com1_coarse_dn", "t154/vhf/com1_coarse_up", "t154/vhf/com1_fine_dn", "t154/vhf/com1_fine_up"};
            break;
        case Radio::VHF2:
            commands = {"t154/vhf/vhf2_mhz_dn", "t154/vhf/vhf2_mhz_up", "t154/vhf/vhf2_khz_dn", "t154/vhf/vhf2_khz_up"};
            break;
        case Radio::VHF3:
            commands = {"t154/vhf/vhf3_mhz_dn", "t154/vhf/vhf3_mhz_up", "t154/vhf/vhf3_khz_dn", "t154/vhf/vhf3_khz_up"};
            break;
        case Radio::NAV1:
            commands = {"sim/radios/actv_nav1_coarse_down", "sim/radios/actv_nav1_coarse_up", "sim/radios/actv_nav1_fine_down", "sim/radios/actv_nav1_fine_up"};
            break;
        case Radio::NAV2:
            commands = {"sim/radios/actv_nav2_coarse_down", "sim/radios/actv_nav2_coarse_up", "sim/radios/actv_nav2_fine_down", "sim/radios/actv_nav2_fine_up"};
            break;
    }

    Dataref::getInstance()->executeCommand(commands[static_cast<int>(knob)]);
}

std::string Tu154CERMPProfile::frequencyText() const {
    auto dataref = Dataref::getInstance();
    char buf[16];
    switch (selectedRadio) {
        case Radio::VHF1:
            snprintf(buf, sizeof(buf), "%.3f", dataref->getCached<int>(kVhf1Frequency) / 1000.0);
            break;
        case Radio::VHF2:
            snprintf(buf, sizeof(buf), "%.3f", dataref->getCached<int>(kVhf2Frequency) / 1000.0);
            break;
        case Radio::VHF3:
            snprintf(buf, sizeof(buf), "%03d.%03d", static_cast<int>(dataref->getCached<double>(kVhf3Mhz)), static_cast<int>(dataref->getCached<double>(kVhf3Khz)));
            break;
        case Radio::NAV1:
            // NAV frequencies are in units of 10 kHz
            snprintf(buf, sizeof(buf), "%.2f", dataref->getCached<int>(kNav1Frequency) / 100.0);
            break;
        case Radio::NAV2:
            snprintf(buf, sizeof(buf), "%.2f", dataref->getCached<int>(kNav2Frequency) / 100.0);
            break;
    }
    return buf;
}

bool Tu154CERMPProfile::isRadioPowered() const {
    auto dataref = Dataref::getInstance();
    switch (selectedRadio) {
        case Radio::VHF1:
            return dataref->getCached<int>(kVhf1Switch) == 1 && dataref->getCached<float>(kLeftBus) > kVhfMinimumVolts;
        case Radio::VHF2:
            return dataref->getCached<int>(kVhf2Switch) == 1 && dataref->getCached<float>(kRightBus) > kVhfMinimumVolts;
        case Radio::VHF3:
            // The aircraft only checks the switch
            return dataref->getCached<double>(kVhf3Switch) > 0;
        case Radio::NAV1:
            return dataref->getCached<int>(kNav1Power) != 0;
        case Radio::NAV2:
            return dataref->getCached<int>(kNav2Power) != 0;
    }
    return false;
}

void Tu154CERMPProfile::updateDisplays() {
    if (!Dataref::getInstance()->getCached<bool>(kAvionicsOn) || !isRadioPowered()) {
        product->setDisplayText(kBlank, kBlank);
        return;
    }

    product->setDisplayText(frequencyText(), kBlank);
}

void Tu154CERMPProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = std::clamp(dataref->getCached<float>(kPedestalLighting) / kPedestalLightingMax, 0.0f, 1.0f);
    product->setLedBrightness(RMPLed::BACKLIGHT, powered ? static_cast<uint8_t>(level * 255) : 0);
    product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, powered ? 255 : 0);
}

void Tu154CERMPProfile::updateLeds() {
    bool powered = Dataref::getInstance()->getCached<bool>(kAvionicsOn);
    product->setLedBrightness(RMPLed::VHF1, powered && selectedRadio == Radio::VHF1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VHF2, powered && selectedRadio == Radio::VHF2 ? 255 : 0);
    product->setLedBrightness(RMPLed::VHF3, powered && selectedRadio == Radio::VHF3 ? 255 : 0);
    product->setLedBrightness(RMPLed::ILS, powered && selectedRadio == Radio::NAV1 ? 255 : 0);
    product->setLedBrightness(RMPLed::VOR, powered && selectedRadio == Radio::NAV2 ? 255 : 0);
}

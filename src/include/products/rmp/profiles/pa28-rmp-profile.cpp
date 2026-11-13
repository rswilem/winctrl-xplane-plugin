#include "pa28-rmp-profile.h"

#include "dataref.h"
#include "logger.hpp"
#include "product-rmp.h"

#include <algorithm>
#include <cstdio>

namespace {
    const char *radioName(PA28RMPRadio radio) {
        switch (radio) {
            case PA28RMPRadio::COM1:
                return "com1";
            case PA28RMPRadio::COM2:
                return "com2";
            case PA28RMPRadio::NAV1:
                return "nav1";
            case PA28RMPRadio::NAV2:
                return "nav2";
            case PA28RMPRadio::ADF1:
                return "adf1";
        }
        return "com1";
    }

    bool isCom(PA28RMPRadio radio) {
        return radio == PA28RMPRadio::COM1 || radio == PA28RMPRadio::COM2;
    }

    // COM 8.33 datarefs are in kHz (122800 -> "122.800")
    std::string formatCom(int khz) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.3f", khz / 1000.0);
        return buf;
    }

    // NAV datarefs are in 10 kHz units (11030 -> "110.30")
    std::string formatNav(int tenKhz) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", tenKhz / 100.0);
        return buf;
    }
} // namespace

PA28RMPProfile::PA28RMPProfile(ProductRMP *product) : RMPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio", [this, product](const std::vector<float> &brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        float panel = 0.0f;
        for (size_t i = 0; i < brightness.size() && i < 4; i++) {
            panel = std::max(panel, brightness[i]);
        }

        uint8_t backlight = hasPower ? panel * 255 : 0;
        uint8_t screens = hasPower ? 255 : 0;

        if (backlight == lastBacklightSent && screens == lastScreensSent) {
            return;
        }
        lastBacklightSent = backlight;
        lastScreensSent = screens;

        product->setLedBrightness(RMPLed::BACKLIGHT, backlight);
        product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, screens);
        product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [this](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
        updateDisplays();
    },
        this);

    Logger::getInstance()->info("RMP: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    updateLeds();
}

bool PA28RMPProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::vector<std::string> &PA28RMPProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "sim/cockpit/electrical/battery_on",
        "sim/cockpit2/radios/actuators/com1_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com1_standby_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_frequency_hz_833",
        "sim/cockpit2/radios/actuators/com2_standby_frequency_hz_833",
        "sim/cockpit2/radios/actuators/nav1_frequency_hz",
        "sim/cockpit2/radios/actuators/nav1_standby_frequency_hz",
        "sim/cockpit2/radios/actuators/nav2_frequency_hz",
        "sim/cockpit2/radios/actuators/nav2_standby_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_frequency_hz",
        "sim/cockpit2/radios/actuators/adf1_standby_frequency_hz",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &PA28RMPProfile::buttonDefs() const {
    // The Airbus panel is mapped onto the PA28 radio stack:
    // VHF1 = COM1, VHF2 = COM2, VOR = NAV1, ILS = NAV2, ADF = ADF1.
    // The "custom_*" actions are resolved against the selected radio in buttonPressed().
    static const std::unordered_map<uint16_t, RMPButtonDef> buttons = {
        {0, {"Flip frequencies", "custom_flip"}},
        {1, {"VHF 1", "custom_select", RMPDatarefType::SET_VALUE, static_cast<double>(PA28RMPRadio::COM1)}},
        {2, {"VHF 2", "custom_select", RMPDatarefType::SET_VALUE, static_cast<double>(PA28RMPRadio::COM2)}},
        {3, {"VHF 3", ""}},
        {4, {"LOAD", ""}},
        {5, {"HF1", ""}},
        {6, {"HF2", ""}},
        {7, {"AM", ""}},
        {8, {"NAV", ""}},
        {9, {"VOR", "custom_select", RMPDatarefType::SET_VALUE, static_cast<double>(PA28RMPRadio::NAV1)}},
        {10, {"ILS", "custom_select", RMPDatarefType::SET_VALUE, static_cast<double>(PA28RMPRadio::NAV2)}},
        {11, {"GLS", ""}},
        {12, {"MLS", ""}},
        {13, {"ADF", "custom_select", RMPDatarefType::SET_VALUE, static_cast<double>(PA28RMPRadio::ADF1)}},
        {14, {"Knob Large L", "custom_coarse_down"}},
        {15, {"Knob Large R", "custom_coarse_up"}},
        {16, {"Knob Small L", "custom_fine_down"}},
        {17, {"Knob Small R", "custom_fine_up"}},
        // COM: toggles the small knob between 8.33 kHz (default) and 25 kHz steps. ADF: between 1 kHz (default) and 10 kHz steps.
        {18, {"Knob depress", "custom_step_toggle"}},
        {19, {"Switch On", "custom_power", RMPDatarefType::SET_VALUE, 1}},
        {20, {"Switch Off", "custom_power", RMPDatarefType::SET_VALUE, 0}},
    };

    return buttons;
}

void PA28RMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    const std::string &action = button->dataref;

    if (action == "custom_power") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        panelOn = button->value > 0.5;
        updateLeds();
        product->updateDisplays(true);
        return;
    }

    if (!panelOn) {
        return;
    }

    if (action == "custom_select") {
        if (phase != xplm_CommandBegin) {
            return;
        }

        selectedRadio = static_cast<PA28RMPRadio>(static_cast<int>(button->value));
        updateLeds();
        product->updateDisplays(true);
        return;
    }

    if (action == "custom_step_toggle") {
        if (phase == xplm_CommandBegin) {
            altFineStep = !altFineStep;
        }
        return;
    }

    std::string radio = radioName(selectedRadio);
    std::string command;

    if (action == "custom_flip") {
        // "standy" is not a typo here, it is how X-Plane names these commands
        command = "sim/radios/" + radio + "_standy_flip";
    } else {
        bool up = action.ends_with("_up");
        bool coarse = action.starts_with("custom_coarse");
        std::string direction = up ? "up" : "down";

        if (selectedRadio == PA28RMPRadio::ADF1) {
            std::string digit = coarse ? "hundreds" : (altFineStep ? "tens" : "ones");
            command = "sim/radios/stby_adf1_" + digit + "_" + direction;
        } else {
            command = "sim/radios/stby_" + radio + (coarse ? "_coarse_" : "_fine_") + direction;
            if (!coarse && !altFineStep && isCom(selectedRadio)) {
                command += "_833";
            }
        }
    }

    Dataref::getInstance()->executeCommand(command.c_str(), phase);
}

void PA28RMPProfile::updateLeds() {
    if (!product) {
        return;
    }

    auto setLed = [this](RMPLed led, PA28RMPRadio radio) {
        product->setLedBrightness(led, panelOn && selectedRadio == radio ? 255 : 0);
    };

    setLed(RMPLed::VHF1, PA28RMPRadio::COM1);
    setLed(RMPLed::VHF2, PA28RMPRadio::COM2);
    setLed(RMPLed::VOR, PA28RMPRadio::NAV1);
    setLed(RMPLed::ILS, PA28RMPRadio::NAV2);
    setLed(RMPLed::ADF, PA28RMPRadio::ADF1);
}

void PA28RMPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    bool hasPower = datarefManager->getCached<bool>("sim/cockpit/electrical/battery_on");
    if (!hasPower || !panelOn) {
        product->setDisplayText("      ", "      ");
        return;
    }

    std::string base = std::string("sim/cockpit2/radios/actuators/") + radioName(selectedRadio);
    std::string suffix = isCom(selectedRadio) ? "frequency_hz_833" : "frequency_hz";

    int active = datarefManager->getCached<int>((base + "_" + suffix).c_str());
    int stby = datarefManager->getCached<int>((base + "_standby_" + suffix).c_str());

    if (isCom(selectedRadio)) {
        product->setDisplayText(formatCom(active), formatCom(stby));
    } else if (selectedRadio == PA28RMPRadio::ADF1) {
        product->setDisplayText(std::to_string(active), std::to_string(stby));
    } else {
        product->setDisplayText(formatNav(active), formatNav(stby));
    }
}

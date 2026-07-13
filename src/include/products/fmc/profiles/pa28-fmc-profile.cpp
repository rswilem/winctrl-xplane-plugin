#include "pa28-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"
#include "logger.hpp"

#include <algorithm>

// Lighting-only profile: the PA28 has no FMS, so the screen stays blank and
// the backlight follows the battery like the rest of the hardware
PA28FMCProfile::PA28FMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
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

        product->setLedBrightness(FMCLed::BACKLIGHT, backlight);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, screens);
        product->setLedBrightness(FMCLed::OVERALL_LEDS_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Logger::getInstance()->info("FMC: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28FMCProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::vector<std::string> &PA28FMCProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::vector<FMCButtonDef> &PA28FMCProfile::buttonDefs() const {
    static const std::vector<FMCButtonDef> buttons = {};
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &PA28FMCProfile::buttonKeyMap() const {
    static const std::unordered_map<FMCKey, const FMCButtonDef *> keyMap = {};
    return keyMap;
}

const std::map<char, FMCTextColor> &PA28FMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colors = {};
    return colors;
}

void PA28FMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
}

void PA28FMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    for (auto &line : page) {
        std::fill(line.begin(), line.end(), 0);
    }
}

void PA28FMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
}

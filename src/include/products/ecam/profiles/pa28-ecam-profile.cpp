#include "pa28-ecam-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ecam.h"
#include "logger.hpp"

#include <algorithm>

// Lighting-only profile: the ECAM panel has no PA28 function, but its backlight
// should follow the battery like the rest of the hardware
PA28ECAMProfile::PA28ECAMProfile(ProductECAM *product) : ECAMAircraftProfile(product) {
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

        product->setLedBrightness(ECAMLed::BACKLIGHT, backlight);
        product->setLedBrightness(ECAMLed::EMER_CANC_BRIGHTNESS, backlight);
        product->setLedBrightness(ECAMLed::OVERALL_LEDS_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Logger::getInstance()->info("ECAM: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28ECAMProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::unordered_map<uint16_t, ECAMButtonDef> &PA28ECAMProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, ECAMButtonDef> buttons = {};
    return buttons;
}

void PA28ECAMProfile::buttonPressed(const ECAMButtonDef *button, XPLMCommandPhase phase) {
}

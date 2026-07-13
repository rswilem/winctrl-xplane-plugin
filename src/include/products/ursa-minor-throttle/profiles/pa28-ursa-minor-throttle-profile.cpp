#include "pa28-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"
#include "logger.hpp"

#include <algorithm>

// Lighting-only profile: throttle axes are handled by X-Plane itself, the
// plugin just keeps the backlight in step with the battery
PA28UrsaMinorThrottleProfile::PA28UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
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

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, backlight);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, screens);
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool batteryOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
    },
        this);

    Logger::getInstance()->info("Ursa Minor Throttle: PA28 profile active\n");
    Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio");
}

bool PA28UrsaMinorThrottleProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return icao.starts_with("P28");
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &PA28UrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {};
    return buttons;
}

void PA28UrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
}

void PA28UrsaMinorThrottleProfile::updateDisplays() {
}

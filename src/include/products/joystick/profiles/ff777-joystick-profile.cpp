#include "ff777-joystick-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-joystick.h"

#include <algorithm>
#include <cmath>

FF777JoystickProfile::FF777JoystickProfile(USBDevice *product) : JoystickAircraftProfile(product) {
    auto joystick = static_cast<ProductJoystick *>(product);
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [joystick](const std::vector<float> &panelBrightness) {
        if (panelBrightness.size() < 4) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t target = hasPower ? panelBrightness[3] * 255 : 0;
        joystick->setLedBrightness(target);

        if (!hasPower) {
            joystick->setVibration(0);
        }
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    }, this);
}

bool FF777JoystickProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/electric/panel_brightness");
}

#include "toliss-joystick-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-joystick.h"

#include <algorithm>
#include <cmath>

TolissJoystickProfile::TolissJoystickProfile(USBDevice *product) : JoystickAircraftProfile(product) {
    auto joystick = static_cast<ProductJoystick *>(product);
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [joystick](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t target = hasPower ? brightness * 255 : 0;
        joystick->setLedBrightness(target);

        if (!hasPower) {
            joystick->setVibration(0);
        }
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
    }, this);
}

bool TolissJoystickProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

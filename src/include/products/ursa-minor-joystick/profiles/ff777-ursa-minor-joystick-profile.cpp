#include "ff777-ursa-minor-joystick-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-joystick.h"

#include <algorithm>
#include <cmath>

FF777UrsaMinorJoystickProfile::FF777UrsaMinorJoystickProfile(ProductUrsaMinorJoystick *product) : UrsaMinorJoystickAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("laminar/B738/electric/panel_brightness", [product](std::vector<float> panelBrightness) {
        if (panelBrightness.size() < 4) {
            return;
        }

        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t target = hasPower ? panelBrightness[3] * 255 : 0;
        product->setLedBrightness(target);

        if (!hasPower) {
            product->setVibration(0);
        }
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/panel_brightness");
    });
}

FF777UrsaMinorJoystickProfile::~FF777UrsaMinorJoystickProfile() {
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("laminar/B738/electric/panel_brightness");
    Dataref::getInstance()->unbind("sim/flightmodel/failures/onground_any");
}

bool FF777UrsaMinorJoystickProfile::IsEligible() {
    return Dataref::getInstance()->exists("laminar/B738/electric/panel_brightness");
}

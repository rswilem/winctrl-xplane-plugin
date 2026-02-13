#include "toliss-ursa-minor-joystick-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-joystick.h"

#include <algorithm>
#include <cmath>

TolissUrsaMinorJoystickProfile::TolissUrsaMinorJoystickProfile(ProductUrsaMinorJoystick *product) : UrsaMinorJoystickAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t target = hasPower ? brightness * 255 : 0;
        product->setLedBrightness(target);

        if (!hasPower) {
            product->setVibration(0);
        }
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
    });
}

TolissUrsaMinorJoystickProfile::~TolissUrsaMinorJoystickProfile() {
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    Dataref::getInstance()->unbind("AirbusFBW/PanelBrightnessLevel");
    Dataref::getInstance()->unbind("sim/flightmodel/failures/onground_any");
}

bool TolissUrsaMinorJoystickProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

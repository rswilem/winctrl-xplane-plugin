#ifndef TOLISS_JOYSTICK_PROFILE_H
#define TOLISS_JOYSTICK_PROFILE_H

#include "joystick-aircraft-profile.h"

#include <string>

class USBDevice;

class TolissJoystickProfile : public JoystickAircraftProfile {
    public:
        TolissJoystickProfile(USBDevice *product);

        static bool IsEligible();
};

#endif

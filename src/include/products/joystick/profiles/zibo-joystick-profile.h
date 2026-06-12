#ifndef ZIBO_JOYSTICK_PROFILE_H
#define ZIBO_JOYSTICK_PROFILE_H

#include "joystick-aircraft-profile.h"

#include <string>

class USBDevice;

class ZiboJoystickProfile : public JoystickAircraftProfile {
    public:
        ZiboJoystickProfile(USBDevice *product);

        static bool IsEligible();
};

#endif

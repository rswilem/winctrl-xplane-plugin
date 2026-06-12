#ifndef FF777_JOYSTICK_PROFILE_H
#define FF777_JOYSTICK_PROFILE_H

#include "joystick-aircraft-profile.h"

#include <string>

class USBDevice;

class FF777JoystickProfile : public JoystickAircraftProfile {
    private:
    public:
        FF777JoystickProfile(USBDevice *product);

        static bool IsEligible();
};

#endif

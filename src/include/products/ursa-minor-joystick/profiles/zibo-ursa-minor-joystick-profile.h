#ifndef ZIBO_URSA_MINOR_JOYSTICK_PROFILE_H
#define ZIBO_URSA_MINOR_JOYSTICK_PROFILE_H

#include "ursa-minor-joystick-aircraft-profile.h"

#include <string>

class ZiboUrsaMinorJoystickProfile : public UrsaMinorJoystickAircraftProfile {
    public:
        ZiboUrsaMinorJoystickProfile(ProductUrsaMinorJoystick *product);
        ~ZiboUrsaMinorJoystickProfile();

        static bool IsEligible();
};

#endif

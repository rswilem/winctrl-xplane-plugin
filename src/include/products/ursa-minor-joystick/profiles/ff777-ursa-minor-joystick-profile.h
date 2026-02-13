#ifndef FF777_URSA_MINOR_JOYSTICK_PROFILE_H
#define FF777_URSA_MINOR_JOYSTICK_PROFILE_H

#include "ursa-minor-joystick-aircraft-profile.h"

#include <string>

class FF777UrsaMinorJoystickProfile : public UrsaMinorJoystickAircraftProfile {
    private:
    public:
        FF777UrsaMinorJoystickProfile(ProductUrsaMinorJoystick *product);
        ~FF777UrsaMinorJoystickProfile();

        static bool IsEligible();
};

#endif

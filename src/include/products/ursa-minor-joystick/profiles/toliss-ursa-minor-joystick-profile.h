#ifndef TOLISS_URSA_MINOR_JOYSTICK_PROFILE_H
#define TOLISS_URSA_MINOR_JOYSTICK_PROFILE_H

#include "ursa-minor-joystick-aircraft-profile.h"

#include <string>

class TolissUrsaMinorJoystickProfile : public UrsaMinorJoystickAircraftProfile {
    public:
        TolissUrsaMinorJoystickProfile(ProductUrsaMinorJoystick *product);
        ~TolissUrsaMinorJoystickProfile();

        static bool IsEligible();
};

#endif

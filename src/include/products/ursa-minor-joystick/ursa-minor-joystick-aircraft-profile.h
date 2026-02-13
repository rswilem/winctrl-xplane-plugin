#ifndef URSA_MINOR_JOYSTICK_AIRCRAFT_PROFILE_H
#define URSA_MINOR_JOYSTICK_AIRCRAFT_PROFILE_H

#include <string>
#include <vector>

class ProductUrsaMinorJoystick;

class UrsaMinorJoystickAircraftProfile {
    protected:
        ProductUrsaMinorJoystick *product;

    public:
        UrsaMinorJoystickAircraftProfile(ProductUrsaMinorJoystick *product) : product(product) {};
        virtual ~UrsaMinorJoystickAircraftProfile() = default;

        virtual void update() {};
};

#endif

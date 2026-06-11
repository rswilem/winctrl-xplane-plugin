#ifndef JOYSTICK_AIRCRAFT_PROFILE_H
#define JOYSTICK_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

#include <string>
#include <vector>

class USBDevice;

class JoystickAircraftProfile {
    protected:
        USBDevice *product;

    public:
        JoystickAircraftProfile(USBDevice *product) : product(product) {};
        virtual ~JoystickAircraftProfile() {
            cleanupProfile(this);
        }

        virtual void update() {};
};

#endif

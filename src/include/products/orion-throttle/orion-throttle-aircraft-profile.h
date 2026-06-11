#ifndef ORION_THROTTLE_AIRCRAFT_PROFILE_H
#define ORION_THROTTLE_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

class USBDevice;

class OrionThrottleAircraftProfile {
    protected:
        USBDevice *product;

    public:
        OrionThrottleAircraftProfile(USBDevice *product) : product(product) {};
        virtual ~OrionThrottleAircraftProfile() {
            cleanupProfile(this);
        }

        virtual void update() {}
};

#endif

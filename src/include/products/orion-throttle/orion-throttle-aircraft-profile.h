#ifndef ORION_THROTTLE_AIRCRAFT_PROFILE_H
#define ORION_THROTTLE_AIRCRAFT_PROFILE_H

class USBDevice;

class OrionThrottleAircraftProfile {
    protected:
        USBDevice *product;

    public:
        OrionThrottleAircraftProfile(USBDevice *product) : product(product) {};
        virtual ~OrionThrottleAircraftProfile() = default;

        virtual void update() {}
};

#endif

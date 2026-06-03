#ifndef PRODUCT_ORION_THROTTLE_H
#define PRODUCT_ORION_THROTTLE_H

#include "orion-throttle-aircraft-profile.h"
#include "usbdevice.h"

class ProductOrionThrottle : public USBDevice {
    private:
        OrionThrottleAircraftProfile *profile;
        int menuItemId;

        int lastVibration = 0;
        float lastGForce = 1.0f;

        void setProfileForCurrentAircraft();
        void loadVibrationSetting(const std::string &preference);

    public:
        ProductOrionThrottle(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        ~ProductOrionThrottle();

        float vibrationMultiplier;

        const char *classIdentifier() override;
        const char *activeProfileName() const override;
        bool connect() override;
        void update() override;
        void blackout() override;

        void setVibration(uint8_t vibration);
};

#endif

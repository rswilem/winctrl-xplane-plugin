#ifndef PRODUCT_URSA_MINOR_JOYSTICK_H
#define PRODUCT_URSA_MINOR_JOYSTICK_H

#include "ursa-minor-joystick-aircraft-profile.h"
#include "usbdevice.h"

class ProductUrsaMinorJoystick : public USBDevice {
    private:
        UrsaMinorJoystickAircraftProfile *profile;
        int menuItemId;

        int lastVibration = 0;
        float lastGForce = 0.0f;

        void setProfileForCurrentAircraft();
        void loadVibrationSetting(const std::string &preference);

    public:
        ProductUrsaMinorJoystick(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, unsigned char identifierByte);
        ~ProductUrsaMinorJoystick();

        const unsigned char identifierByte;
        float vibrationMultiplier;

        const char *classIdentifier() override;
        bool connect() override;
        void update() override;
        void blackout() override;

        void setVibration(uint8_t vibration);
        void setLedBrightness(uint8_t brightness);
};

#endif

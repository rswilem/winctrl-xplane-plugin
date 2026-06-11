#ifndef PRODUCT_JOYSTICK_H
#define PRODUCT_JOYSTICK_H

#include "joystick-aircraft-profile.h"
#include "usbdevice.h"

class ProductJoystick : public USBDevice {
    private:
        JoystickAircraftProfile *profile;
        int menuItemId;

        int lastVibration = 0;
        float lastGForce = 1.0f;

        void setProfileForCurrentAircraft();
        void loadVibrationSetting(const std::string &preference);

    public:
        ProductJoystick(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, unsigned char identifierByte, unsigned char motorCode);
        ~ProductJoystick();

        const unsigned char identifierByte;
        const unsigned char motorCode;
        // 1.0f lets the initial setVibration(0) in connect() write the
        // motor-off packet; loadVibrationSetting() overrides it right after.
        float vibrationMultiplier = 1.0f;

        const char *classIdentifier() override;
        const char *activeProfileName() const override;
        bool connect() override;
        void update() override;
        void blackout() override;

        void setVibration(uint8_t vibration);
        void testVibration(uint8_t testIdentifier, uint8_t motorCode);
        void setLedBrightness(uint8_t brightness);
};

#endif

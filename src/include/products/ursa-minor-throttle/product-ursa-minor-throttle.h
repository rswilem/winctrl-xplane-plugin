#ifndef PRODUCT_URSA_MINOR_THROTTLE_H
#define PRODUCT_URSA_MINOR_THROTTLE_H

#include "ursa-minor-throttle-aircraft-profile.h"
#include "usbdevice.h"

#include <set>

enum class UrsaMinorThrottleLed : int {
    BACKLIGHT = 0,
    OVERALL_LEDS_AND_LCD_BRIGHTNESS = 2,

    _START = 3,
    ENG_1_FAULT = 3,
    ENG_1_FIRE = 4,
    ENG_2_FAULT = 5,
    ENG_2_FIRE = 6,
    _END = 6,
};

class ProductUrsaMinorThrottle : public USBDevice {
    private:
        UrsaMinorThrottleAircraftProfile *profile;
        int menuItemId;
        uint64_t lastButtonStateLo;
        uint32_t lastButtonStateHi;
        std::set<int> pressedButtonIndices;
        uint8_t packetNumber = 1;

        int lastVibration = 0;
        float lastGForce = 1.0f;

        void setProfileForCurrentAircraft();
        void loadVibrationSetting(const std::string &preference);

    public:
        ProductUrsaMinorThrottle(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        ~ProductUrsaMinorThrottle();

        static constexpr unsigned char ThrottleIdentifierByte = 0x10;
        static constexpr unsigned char PACIdentifierByte = 0x01;
        float vibrationMultiplier;

        const char *classIdentifier() override;
        bool connect() override;
        void update() override;
        void blackout() override;
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;
        void forceStateSync() override;

        void setAllLedsEnabled(bool enabled);
        void setLedBrightness(UrsaMinorThrottleLed led, uint8_t brightness);
        void setVibration(uint8_t vibration, bool leftSide = true, bool rightSide = true);
        void setLCDText(const std::string &text);
};

#endif

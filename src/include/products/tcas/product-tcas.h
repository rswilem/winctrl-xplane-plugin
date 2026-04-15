#ifndef PRODUCT_TCAS_H
#define PRODUCT_TCAS_H

#include "tcas-aircraft-profile.h"
#include "usbdevice.h"

#include <set>

enum class TCASLed : int {
    BACKLIGHT = 0,
    LCD_BRIGHTNESS = 1,
    OVERALL_LEDS_BRIGHTNESS = 2,

    _START = 3,
    ATC_FAIL = 3,
    _END = 3,
};

class ProductTCAS : public USBDevice {
    private:
        TCASAircraftProfile *profile;
        int menuItemId;
        int displayUpdateFrameCounter = 0;
        uint64_t lastButtonStateLo;
        uint32_t lastButtonStateHi;
        std::set<int> pressedButtonIndices;
        uint8_t packetNumber = 1;

        void setProfileForCurrentAircraft();

    public:
        ProductTCAS(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName);
        ~ProductTCAS();

        static constexpr unsigned char IdentifierByte = 0x99;

        const char *classIdentifier() override;
        bool connect() override;
        void update() override;
        void blackout() override;
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;

        void setAllLedsEnabled(bool enabled);
        void setLedBrightness(TCASLed led, uint8_t brightness);
        void setLCDText(const std::string &squawkCode);
};

#endif

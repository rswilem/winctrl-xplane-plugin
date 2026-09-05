#ifndef PRODUCT_PDC_H
#define PRODUCT_PDC_H

#include "pdc-aircraft-profile.h"
#include "usbdevice.h"

#include <set>

enum class PDCLed : int {
    BACKLIGHT = 0
};

class ProductPDC : public USBDevice {
    private:
        PDCAircraftProfile *profile = nullptr;
        int menuItemId = -1;
        uint64_t lastButtonStateLo = 0;
        uint32_t lastButtonStateHi = 0;
        std::set<int> pressedButtonIndices;

        void setProfileForCurrentAircraft();
        void unloadProfile();
        const PDCButtonDef *buttonDefForIndex(uint16_t hardwareButtonIndex) const;
        // Keyed on the physical panel, not on the side it drives, so an L and
        // an R unit keep their own setting.
        std::string variantPreferenceKey() const;
        std::string positionName() const;

    public:
        ProductPDC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, PDCDeviceVariant variant, unsigned char identifierByte);
        ~ProductPDC();

        const unsigned char identifierByte;
        // What the unit reports over USB. Identifies the physical panel and the
        // 3N/3M model the hardware button indices depend on, so it never
        // changes; only the side below does.
        const PDCDeviceVariant hardwareVariant;
        // Which cockpit side the panel drives. Defaults to the side the product
        // ID implies, overridable from the plugins menu.
        PDCDeviceVariant deviceVariant;

        bool isCaptainSide() const;
        bool isHardware3N() const;

        const char *classIdentifier() override;
        const char *activeProfileName() const override;
        bool connect() override;
        void update() override;
        void blackout() override;
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;
        void forceStateSync() override;

        void setDeviceVariant(PDCDeviceVariant variant);
        void setLedBrightness(PDCLed led, uint8_t brightness);
};

#endif

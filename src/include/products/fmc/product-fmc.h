#ifndef PRODUCT_FMC_H
#define PRODUCT_FMC_H

#include "fmc-aircraft-profile.h"
#include "font.h"
#include "usbdevice.h"

#include <chrono>
#include <map>
#include <set>

class ProductFMC : public USBDevice {
    private:
        FMCAircraftProfile *profile;
        std::vector<std::vector<char>> page;
        int lastUpdateCycle;
        int displayUpdateFrameCounter = 0;
        std::set<int> pressedButtonIndices;
        uint64_t lastButtonStateLo;
        uint32_t lastButtonStateHi;
        int menuItemId;
        int fontsMenuItemId;
        FontVariant preferredFontVariant = FontVariant::Default;

        void draw(const std::vector<std::vector<char>> *pagePtr = nullptr);
        std::pair<uint8_t, uint8_t> dataFromColFont(char color, bool fontSmall = false);

        void setProfileForCurrentAircraft();

        // SimAppPro "Screen Position": the top-left corner of the screen content.
        // Sends one 0x2a packet carrying the 0x18 grid block with left = 36+x,
        // top = 20+y. Always applied as part of setScreenLayout (X/Y belong with the
        // character size).
        void setScreenPosition(unsigned char x, unsigned char y);

    public:
        ProductFMC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, FMCHardwareType hardwareType, FMCDeviceVariant variant, unsigned char identifierByte);
        ~ProductFMC();

        static constexpr unsigned int PageLines = 14; // Header + 6 * label + 6 * cont + textbox
        static constexpr unsigned int PageCharsPerLine = 24;
        static constexpr unsigned int PageBytesPerChar = 3;
        static constexpr unsigned int PageBytesPerLine = PageCharsPerLine * PageBytesPerChar;
        FMCHardwareType hardwareType;
        const unsigned char identifierByte;
        const FMCDeviceVariant deviceVariant;

        const char *classIdentifier() override;
        const char *activeProfileName() const override;
        bool connect() override;
        void unloadProfile();
        void update() override;
        void blackout() override;
        void updatePage(bool forceUpdate = false);
        void didReceiveData(int reportId, uint8_t *report, int reportLength) override;
        void didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count = 1) override;

        char getPageCharacter(std::vector<std::vector<char>> &page, int line, int pos);
        void writeLineToPage(std::vector<std::vector<char>> &page, int line, int pos, const std::string &text, char color, bool fontSmall = false);
        void setFont(FontVariant preferredVariant);

        // Apply the SimAppPro "Screen Layout Settings" as one unit: Character Size
        // (width x height of each character) plus Screen Position (top-left x/y). Used
        // for PFP devices whose 14 display rows must line up with the physical LSK keys.
        // Defaults are the MCDU spec (character 23 x 29, position 16/17).
        void setScreenLayout(FontVariant variant, unsigned char characterHeight = 29, unsigned char characterWidth = 23, unsigned char x = 16, unsigned char y = 17);

        void setAllLedsEnabled(bool enable);
        void setLedBrightness(FMCLed led, uint8_t brightness);

        void clearDisplay();
        void showBackground(FMCBackgroundVariant variant);

        void setDeviceVariant(FMCDeviceVariant variant);

        void reloadFontsMenu();
};

#endif

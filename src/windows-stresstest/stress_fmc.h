#pragma once

#include "usbdevice.h"

#include <cstdint>
#include <string>
#include <vector>

// LED index constants — subset of FMCLed from fmc-aircraft-profile.h.
enum StressFMCLed : uint8_t {
    STRESS_LED_BACKLIGHT         = 0,
    STRESS_LED_SCREEN_BACKLIGHT  = 1,
    STRESS_LED_OVERALL_BRIGHTNESS = 2,
    STRESS_LED_MCDU_START        = 8,
    STRESS_LED_MCDU_END          = 16,
};

// Subclass of USBDevice that drives an MCDU device for the stress test.
// It uses USBDevice::connect() / writeData() / disconnect() from usbdevice_win.cpp
// exactly as-is. Only the FMC-specific init sequence and display logic live here.
class StressFMC : public USBDevice {
public:
    static constexpr unsigned int PageLines        = 14;
    static constexpr unsigned int PageCharsPerLine = 24;
    static constexpr unsigned int PageBytesPerChar = 3;
    static constexpr unsigned int PageBytesPerLine = PageCharsPerLine * PageBytesPerChar;
    static constexpr uint8_t      identifierByte   = 0x32; // MCDU hardware identifier

    StressFMC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId,
              std::string vendorName, std::string productName);
    ~StressFMC() override;

    bool connect() override;
    const char *classIdentifier() override;

    // Update the display with a scrolling Lorem Ipsum. Each row starts at
    // (scrollOffset + rowIndex) in the text, so every row is different and
    // the device must repaint the full screen each call.
    void drawScrollingText(int scrollOffset);

    void setLedBrightness(uint8_t led, uint8_t brightness);
    void setAllLedsEnabled(bool enable);

private:
    void sendInitPackets();
    void sendFont();
    void showBackground_WINCTRL_LOGO();
};

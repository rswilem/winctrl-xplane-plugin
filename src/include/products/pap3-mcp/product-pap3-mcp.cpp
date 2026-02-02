#include "product-pap3-mcp.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "pap3-mcp-lcd-segments.h"
#include "plugins-menu.h"
#include "profiles/ff777-pap3-mcp-profile.h"
#include "profiles/laminar-pap3-mcp-profile.h"
#include "profiles/rotatemd11-pap3-mcp-profile.h"
#include "profiles/zibo-pap3-mcp-profile.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <XPLMDataAccess.h>
#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

using namespace pap3mcp::lcd;

ProductPAP3MCP::ProductPAP3MCP(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) :
    USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    profile = nullptr;
    displayData = {};
    lastUpdateCycle = 0;
    pressedButtonIndices = {};

    connect();
}

ProductPAP3MCP::~ProductPAP3MCP() {
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

void ProductPAP3MCP::setProfileForCurrentAircraft() {
    if (ZiboPAP3MCPProfile::IsEligible()) {
        profile = new ZiboPAP3MCPProfile(this);
        profileReady = true;
    } else if (FF777PAP3MCPProfile::IsEligible()) {
        profile = new FF777PAP3MCPProfile(this);
        profileReady = true;
    } else if (RotateMD11PAP3MCPProfile::IsEligible()) {
        profile = new RotateMD11PAP3MCPProfile(this);
        profileReady = true;
    } else if (LaminarPAP3MCPProfile::IsEligible()) {
        profile = new LaminarPAP3MCPProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

const char *ProductPAP3MCP::classIdentifier() {
    return "PAP3-MCP";
}

bool ProductPAP3MCP::connect() {
    if (USBDevice::connect()) {
        initializeDisplays();

        setLedBrightness(PAP3MCPLed::BACKLIGHT, 0);
        setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
        setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 0);

        menuItemId = PluginsMenu::getInstance()->addItem(
            classIdentifier(),
            std::vector<MenuItem>{
                {.name = "Identify", .content = [this](int menuId) {
                     setLedBrightness(PAP3MCPLed::BACKLIGHT, 255);
                     setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 255);
                     setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 255);
                     setAllLedsEnabled(true);
                     AppState::getInstance()->executeAfter(2000, [this]() {
                         setLedBrightness(PAP3MCPLed::BACKLIGHT, 128);
                         setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 128);
                         setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 128);
                         setAllLedsEnabled(false);
                     });
                 }},
            });

        if (!profile) {
            setProfileForCurrentAircraft();
        }

        return true;
    }

    return false;
}

void ProductPAP3MCP::blackout() {
    // Turn off all LEDs
    setLedBrightness(PAP3MCPLed::BACKLIGHT, 0);
    setLedBrightness(PAP3MCPLed::LCD_BACKLIGHT, 0);
    setLedBrightness(PAP3MCPLed::OVERALL_LED_BRIGHTNESS, 0);

    setAllLedsEnabled(false);

    clearDisplays();
}

void ProductPAP3MCP::update() {
    if (!connected) {
        return;
    }

    if (!profile) {
        setProfileForCurrentAircraft();
        return;
    }

    USBDevice::update();

    if (++displayUpdateFrameCounter >= getDisplayUpdateFrameInterval()) {
        displayUpdateFrameCounter = 0;
        updateDisplays(false);
    }
}

void ProductPAP3MCP::updateDisplays(bool force) {
    if (!connected || !profile) {
        return;
    }

    bool shouldUpdate = force;
    auto datarefManager = Dataref::getInstance();
    for (const std::string &dataref : profile->displayDatarefs()) {
        if (!lastUpdateCycle || datarefManager->getCachedLastUpdate(dataref.c_str()) > lastUpdateCycle) {
            shouldUpdate = true;
            break;
        }
    }

    if (!shouldUpdate) {
        return;
    }

    // Save old display data for comparison
    PAP3MCPDisplayData oldDisplayData = displayData;
    profile->updateDisplayData(displayData);

    // Update LCD if data changed
    if (displayData.speed != oldDisplayData.speed ||
        displayData.heading != oldDisplayData.heading ||
        displayData.altitude != oldDisplayData.altitude ||
        displayData.verticalSpeed != oldDisplayData.verticalSpeed ||
        displayData.verticalSpeedVisible != oldDisplayData.verticalSpeedVisible ||
        displayData.speedVisible != oldDisplayData.speedVisible ||
        displayData.headingVisible != oldDisplayData.headingVisible ||
        displayData.crsCapt != oldDisplayData.crsCapt ||
        displayData.crsFo != oldDisplayData.crsFo ||
        displayData.showCourse != oldDisplayData.showCourse ||
        displayData.showLabels != oldDisplayData.showLabels ||
        displayData.showDashesWhenInactive != oldDisplayData.showDashesWhenInactive ||
        displayData.showLabelsWhenInactive != oldDisplayData.showLabelsWhenInactive ||
        displayData.digitA != oldDisplayData.digitA ||
        displayData.digitB != oldDisplayData.digitB ||
        displayData.displayEnabled != oldDisplayData.displayEnabled ||
        displayData.displayTest != oldDisplayData.displayTest) {
        // Pass values directly - encoding happens in sendLCDDisplay
        sendLCDDisplay("", displayData.heading, displayData.altitude, "", displayData.crsCapt, displayData.crsFo);
    }

    if (shouldUpdate) {
        lastUpdateCycle = XPLMGetCycleNumber();
    }
}

void ProductPAP3MCP::initializeDisplays() {
    // Send LCD initialization command (opcode 0x12)
    // Structure from working code:
    // - Bytes 0-3: Header [F0 00 SEQ 12]
    // - Bytes 4-23: Init tail [0F BF 00 00 04 01 00 00 26 CC 00 00 00 01 00 00 00 03 00 00]
    // - Bytes 24-63: Padding zeros

    std::vector<uint8_t> initCmd(64, 0x00);

    // Header
    initCmd[0] = 0xF0;
    initCmd[1] = 0x00;
    initCmd[2] = packetNumber;
    initCmd[3] = 0x12; // Init opcode

    // Init tail (starts at offset 4)
    initCmd[4] = 0x0F;
    initCmd[5] = 0xBF;
    initCmd[6] = 0x00;
    initCmd[7] = 0x00;
    initCmd[8] = 0x04;
    initCmd[9] = 0x01;
    initCmd[10] = 0x00;
    initCmd[11] = 0x00;
    initCmd[12] = 0x26;
    initCmd[13] = 0xCC;
    initCmd[14] = 0x00;
    initCmd[15] = 0x00;
    initCmd[16] = 0x00;
    initCmd[17] = 0x01;
    initCmd[18] = 0x00;
    initCmd[19] = 0x00;
    initCmd[20] = 0x00;
    initCmd[21] = 0x03;
    initCmd[22] = 0x00;
    initCmd[23] = 0x00;

    writeData(initCmd);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }
}

void ProductPAP3MCP::clearDisplays() {
    displayData = {
        .displayEnabled = false,
    };

    sendLCDDisplay("", 0, 0, "", 0, 0);
}

void ProductPAP3MCP::sendLCDDisplay(const std::string &speed, int heading, int altitude, const std::string &vs, int crsCapt, int crsFo) {
    Payload payload;
    std::fill(payload.begin(), payload.end(), 0x00);

    if (!displayData.displayEnabled || displayData.displayTest) {
        // Send empty payload
        std::fill(payload.begin(), payload.end(), displayData.displayEnabled && displayData.displayTest ? 0xFF : 0x00);
    } else {
        // SPD: IAS vs MACH rendering
        const float spd = displayData.speed;
        const bool isMach = (spd < 100.0f);

        if (displayData.speedVisible && isMach) {
            // MACH mode
            float mach = (spd < 1.0f) ? std::clamp(spd, 0.0f, 0.9999f) : std::clamp(spd / 100.0f, 0.0f, 0.9999f);
            const int twoDigits = std::clamp(static_cast<int>(std::floor(mach * 1000.0f / 10.0f + 0.5f)), 0, 99);
            const int tens = (twoDigits / 10) % 10;
            const int units = twoDigits % 10;

            drawDigit(G0, payload, SPD_TENS, tens);
            drawDigit(G0, payload, SPD_UNITS, units);

            setFlag(payload, OFF_36, LBL_IAS, displayData.showLabels && false);
            setFlag(payload, OFF_32, LBL_MACH_L, displayData.showLabels && true);
            setFlag(payload, OFF_2E, LBL_MACH_R, displayData.showLabels && true);
            setFlag(payload, OFF_19, DOT_SPD, true);

            setFlag(payload, OFF_22, SPD_BAR_TOP, displayData.digitA);
            setFlag(payload, OFF_1E, SPD_BAR_BOTTOM, displayData.digitA);
        } else if (displayData.speedVisible) {
            // IAS mode
            const int ias = std::max(0, static_cast<int>(std::floor(spd + 0.5f)));
            int k, h, t, u;
            digits4(ias, k, h, t, u);

            const bool showK = (k != 0);
            const bool showH = showK || (h != 0);

            if (showK) {
                drawDigit(G0, payload, SPD_KILO, k);
            }
            if (showH) {
                drawDigit(G0, payload, SPD_HUNDREDS, h);
            }
            drawDigit(G0, payload, SPD_TENS, t);
            drawDigit(G0, payload, SPD_UNITS, u);

            setFlag(payload, OFF_36, LBL_IAS, displayData.showLabels && true);
            setFlag(payload, OFF_32, LBL_MACH_L, displayData.showLabels && false);
            setFlag(payload, OFF_2E, LBL_MACH_R, displayData.showLabels && false);
            setFlag(payload, OFF_19, DOT_SPD, false);

            setFlag(payload, OFF_22, SPD_BAR_TOP, displayData.digitA);
            setFlag(payload, OFF_1E, SPD_BAR_BOTTOM, displayData.digitA);

            // Special digits
            if (!showK) {
                if (displayData.digitA) {
                    drawLetterA(G0, payload, SPD_KILO);
                }
                if (displayData.digitB) {
                    drawDigit(G0, payload, SPD_KILO, 8);
                }
            }
        }

        // CAPT CRS: 3 digits
        if (displayData.showCourse) {
            int h, t, u;
            digits3(std::max(0, crsCapt), h, t, u);
            drawDigit(G0, payload, CPT_CRS_HUNDREDS, h);
            drawDigit(G0, payload, CPT_CRS_TENS, t);
            drawDigit(G0, payload, CPT_CRS_UNITS, u);
            // No dot for CRS displays
            setFlag(payload, OFF_19, DOT_CPT_CRS, false);
        }

        // HDG: 3 digits - only draw if heading is visible
        if (displayData.headingVisible) {
            int h, t, u;
            // Normalize heading: 360 should display as 360, then wrap to 0
            int hdg = (heading >= 360) ? 360 : std::clamp(heading, 0, 359);
            digits3(hdg, h, t, u);
            drawDigit(G1, payload, HDG_HUNDREDS, h);
            drawDigit(G1, payload, HDG_TENS, t);
            drawDigit(G1, payload, HDG_UNITS, u);
            // No dot for HDG display
            setFlag(payload, OFF_26, DOT_HDG, false);
            setFlag(payload, OFF_36, LBL_HDG_L, displayData.showLabels && true);
            setFlag(payload, OFF_32, LBL_HDG_R, displayData.showLabels && true);
            setFlag(payload, OFF_2E, LBL_TRK_L, displayData.showLabels && false);
            setFlag(payload, OFF_2A, LBL_TRK_R, displayData.showLabels && false);
        }

        // ALT: 5 digits
        {
            int d10k, dk, dh, dt, du;
            digits5(std::max(0, altitude), d10k, dk, dh, dt, du);

            const bool show10k = (d10k != 0);

            if (show10k) {
                drawDigit(G1, payload, ALT_TENS_KILO, d10k);
            }
            drawDigit(G1, payload, ALT_KILO, dk);
            drawDigit(G1, payload, ALT_HUNDREDS, dh);
            drawDigit(G2, payload, ALT_TENS, dt);
            drawDigit(G2, payload, ALT_UNITS, du);
            // No dot for ALT display
            setFlag(payload, OFF_1A, DOT_ALT, false);
        }

        // VVI: sign + 4 digits
        if (displayData.verticalSpeedVisible) {
            const int v = static_cast<int>(displayData.verticalSpeed);
            const int absV = std::clamp(std::abs(v), 0, 9999);
            int k, h, t, u;
            digits4(absV, k, h, t, u);

            if (absV >= 1000) {
                drawDigit(G2, payload, VSPD_KILO, k);
            }
            if (absV >= 100) {
                drawDigit(G2, payload, VSPD_HUNDREDS, h);
            }
            if (absV >= 10) {
                drawDigit(G2, payload, VSPD_TENS, t);
            }
            if (absV >= 1) {
                drawDigit(G2, payload, VSPD_UNITS, u);
            }

            const bool neg = (v < 0);
            const bool pos = (v > 0);
            setFlag(payload, OFF_1F, VSPD_MINUS, neg || pos);
            setFlag(payload, OFF_2C, VSPD_PLUS_TOP, pos);
            setFlag(payload, OFF_28, VSPD_PLUS_BOT, pos);
            // No dot for VSPD display
            setFlag(payload, OFF_1B, DOT_VSPD, false);

            // Show V/S label whenever the display is visible and labels are enabled
            setFlag(payload, OFF_38, LBL_VS, displayData.showLabels);
            setFlag(payload, OFF_34, LBL_FPA, false);
        } else if (displayData.showDashesWhenInactive) {
            // V/S display is inactive - only draw dashes if configured
            drawVviDashes(payload);
            // Show VS label even when inactive if configured
            if (displayData.showLabelsWhenInactive) {
                setFlag(payload, OFF_38, LBL_VS, true);
            }
        } else if (displayData.showLabelsWhenInactive) {
            // Just show the label without dashes
            setFlag(payload, OFF_38, LBL_VS, true);
        }

        // FO CRS: 3 digits
        if (displayData.showCourse) {
            int h, t, u;
            digits3(std::max(0, crsFo), h, t, u);
            drawDigit(G3, payload, FO_CRS_HUNDREDS, h);
            drawDigit(G3, payload, FO_CRS_TENS, t);
            drawDigit(G3, payload, FO_CRS_UNITS, u);
            // No dot for FO CRS display
            setFlag(payload, OFF_1C, DOT_FO_CRS, false);
        }

        // Draw dashes for inactive displays if enabled
        if (displayData.showDashesWhenInactive) {
            if (!displayData.speedVisible) {
                drawSpdDashes(payload);
                // Show labels even when inactive if configured
                if (displayData.showLabelsWhenInactive) {
                    setFlag(payload, OFF_36, LBL_IAS, true);
                }
            }
            if (!displayData.headingVisible) {
                drawHdgDashes(payload);
                // Show HDG label even when inactive if configured
                if (displayData.showLabelsWhenInactive) {
                    setFlag(payload, OFF_36, LBL_HDG_L, true);
                    setFlag(payload, OFF_32, LBL_HDG_R, true);
                }
            }
            // Note: V/S dashes are now handled in the main VVI section above
        }
    }

    // Send LCD payload command (opcode 0x38)
    // Packet structure (verified against working implementation):
    // - Bytes 0-3:   Header [F0 00 SEQ 38]
    // - Bytes 4-17:  Preamble [0F BF 00 00 02 01 00 00 DF A2 50 00 00 B0] (14 bytes)
    // - Bytes 18-24: Padding (7 zero bytes to reach offset 25/0x19)
    // - Bytes 25-56: 32-byte LCD segment payload (0x19 to 0x38 inclusive)
    // - Bytes 57-63: Padding zeros to reach 64 bytes total
    std::vector<uint8_t> data;
    data.reserve(64);

    // Header (4 bytes at offset 0-3)
    data.push_back(0xF0);
    data.push_back(0x00);
    data.push_back(packetNumber);
    data.push_back(0x38); // Opcode for LCD payload

    // Preamble (14 bytes at offset 4-17)
    data.push_back(0x0F);
    data.push_back(0xBF);
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x02);
    data.push_back(0x01);
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0xDF);
    data.push_back(0xA2);
    data.push_back(0x50);
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0xB0);

    // Padding (7 bytes at offset 18-24 to reach payload start at 0x19=25)
    for (int i = 0; i < 7; i++) {
        data.push_back(0x00);
    }

    // Now at offset 25 (0x19) - add the 32-byte segment payload
    data.insert(data.end(), payload.begin(), payload.end());

    // Pad to 64 bytes total
    while (data.size() < 64) {
        data.push_back(0x00);
    }

    writeData(data);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }

    // Send two empty frames (opcode 0x38 with no payload)
    for (int i = 0; i < 2; i++) {
        std::vector<uint8_t> emptyFrame(64, 0x00);
        emptyFrame[0] = 0xF0;
        emptyFrame[1] = 0x00;
        emptyFrame[2] = packetNumber;
        emptyFrame[3] = 0x38; // Same opcode

        writeData(emptyFrame);
        if (++packetNumber == 0) {
            packetNumber = 1;
        }
    }

    // Send commit frame (opcode 0x2A)
    std::vector<uint8_t> commitFrame(64, 0x00);
    commitFrame[0] = 0xF0;
    commitFrame[1] = 0x00;
    commitFrame[2] = packetNumber;
    commitFrame[3] = 0x2A; // Commit opcode

    // Add commit constants at specific offsets (from working code)
    commitFrame[0x1D] = 0x0F;
    commitFrame[0x1E] = 0xBF;
    commitFrame[0x21] = 0x03;
    commitFrame[0x22] = 0x01;
    commitFrame[0x25] = 0xDF;
    commitFrame[0x26] = 0xA2;
    commitFrame[0x27] = 0x50;

    writeData(commitFrame);
    if (++packetNumber == 0) {
        packetNumber = 1;
    }
}

void ProductPAP3MCP::setAllLedsEnabled(bool enable) {
    unsigned char start = static_cast<unsigned char>(PAP3MCPLed::_START);
    unsigned char end = static_cast<unsigned char>(PAP3MCPLed::_END);

    for (unsigned char i = start; i <= end; ++i) {
        PAP3MCPLed led = static_cast<PAP3MCPLed>(i);
        setLedBrightness(led, enable ? 1 : 0);
    }
}

void ProductPAP3MCP::setLedBrightness(PAP3MCPLed led, uint8_t brightness) {
    int ledValue = static_cast<int>(led);

    // 14-byte command structure for both dimming and LED control:
    // [0]=0x02 [1]=0x0F [2]=0xBF [3-4]=0x00 [5]=0x03 [6]=0x49 [7]=selector [8]=value [9-13]=0x00
    std::vector<uint8_t> data = {
        0x02, // Report ID
        0x0F, // Header byte 1
        0xBF, // Header byte 2
        0x00,
        0x00,                           // Zeros
        0x03,                           // Constant
        0x49,                           // Constant
        static_cast<uint8_t>(ledValue), // Selector (channel or LED ID)
        brightness,                     // Value (0-255 for dimming, 0x00/0x01 for LED on/off)
        0x00,
        0x00,
        0x00,
        0x00,
        0x00 // Padding
    };

    // For dimming channels (0-2), brightness is 0-255
    // For individual LEDs (3+), brightness should be converted to 0x00 or 0x01
    if (ledValue >= 3) {
        // Individual LED - convert brightness to binary on/off
        data[8] = (brightness > 0) ? 0x01 : 0x00;
    }

    writeData(data);
}

void ProductPAP3MCP::setATSolenoid(bool engaged) {
    std::vector<uint8_t> data = {
        0x02,
        0x0F,
        0xBF,
        0x00,
        0x00,
        0x03,
        0x49,
        0x1E,
        engaged ? static_cast<uint8_t>(0x01) : static_cast<uint8_t>(0x00),
        0x00,
        0x00,
        0x00,
        0x00,
        0x00};

    writeData(data);
}

void ProductPAP3MCP::forceStateSync() {
    pressedButtonIndices.clear();
    lastButtonStateLo = 0;
    lastButtonStateHi = 0;

    USBDevice::forceStateSync();
}

void ProductPAP3MCP::didReceiveData(int reportId, uint8_t *report, int reportLength) {
    if (!connected || !profile || !report || reportLength <= 0) {
        return;
    }

    if (reportId != 1 || reportLength < 32) {
        return;
    }

    uint64_t buttonsLo = 0;
    uint32_t buttonsHi = 0;
    for (int i = 0; i < 8 && i + 1 < reportLength; ++i) {
        buttonsLo |= ((uint64_t) report[i + 1]) << (8 * i);
    }
    for (int i = 0; i < 4 && i + 9 < reportLength; ++i) {
        buttonsHi |= ((uint32_t) report[i + 9]) << (8 * i);
    }

    if (buttonsLo == lastButtonStateLo && buttonsHi == lastButtonStateHi) {
        return;
    }

    lastButtonStateLo = buttonsLo;
    lastButtonStateHi = buttonsHi;

    static uint8_t lastSwitchBytes[7] = {0}; // Track switch states for bytes 0x04-0x06
    static const std::pair<uint8_t, uint8_t> switches[] = {
        {0x04, 0x08}, // FD CAPT (OFF line)
        {0x04, 0x20}, // FD FO (OFF line)
        {0x04, 0x80}, // AP DISCONNECT UP
        {0x05, 0x01}, // AP DISCONNECT DOWN
        {0x06, 0x01}, // A/T ARMED
        {0x06, 0x02}  // A/T DISARMED
    };

    for (const auto &sw : switches) {
        uint8_t byteOffset = sw.first;
        uint8_t bitMask = sw.second;

        if (byteOffset >= reportLength) {
            continue;
        }

        bool currentState = (report[byteOffset] & bitMask) != 0;
        bool lastState = (lastSwitchBytes[byteOffset] & bitMask) != 0;

        if (currentState != lastState) {
            profile->handleSwitchChanged(byteOffset, bitMask, currentState);
        }
    }

    for (int i = 4; i <= 6 && i < reportLength; i++) {
        lastSwitchBytes[i] = report[i];
    }

    for (int byteIndex = 1; byteIndex <= 6 && byteIndex < reportLength; byteIndex++) {
        uint8_t buttonByte = report[byteIndex];

        for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
            int hardwareButtonIndex = (byteIndex - 1) * 8 + bitIndex;
            bool pressed = (buttonByte & (1 << bitIndex)) != 0;
            didReceiveButton(hardwareButtonIndex, pressed);
        }
    }

    // Process bank angle switch (byte 0x05)
    static uint8_t lastBankAngleByte = 0;
    if (reportLength > 0x05) {
        uint8_t currentBankAngleByte = report[0x05];
        if (currentBankAngleByte != lastBankAngleByte) {
            profile->handleBankAngleSwitch(currentBankAngleByte);
            lastBankAngleByte = currentBankAngleByte;
        }
    }

    // Process encoder changes (bytes starting from offset 0x15)
    const std::vector<PAP3MCPEncoderDef> &currentEncoderDefs = profile->encoderDefs();

    // Encoder positions are at specific byte offsets (0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F)
    static const uint8_t encoderOffsets[] = {0x15, 0x17, 0x19, 0x1B, 0x1D, 0x1F};
    static uint8_t lastEncoderPos[6] = {0};

    for (int i = 0; i < 6 && i < currentEncoderDefs.size(); i++) {
        if (encoderOffsets[i] < reportLength) {
            uint8_t currentPos = report[encoderOffsets[i]];
            int8_t delta = static_cast<int8_t>(currentPos - lastEncoderPos[i]);

            if (delta != 0) {
                profile->encoderRotated(&currentEncoderDefs[i], delta);
                lastEncoderPos[i] = currentPos;
            }
        }
    }
}

void ProductPAP3MCP::didReceiveButton(uint16_t hardwareButtonIndex, bool pressed, uint8_t count) {
    USBDevice::didReceiveButton(hardwareButtonIndex, pressed, count);

    if (!connected || !profile) {
        return;
    }

    auto &buttons = profile->buttonDefs();
    auto it = buttons.find(hardwareButtonIndex);
    if (it == buttons.end()) {
        return;
    }

    const PAP3MCPButtonDef *buttonDef = &it->second;

    if (buttonDef->dataref.empty()) {
        return;
    }

    bool pressedButtonIndexExists = pressedButtonIndices.find(hardwareButtonIndex) != pressedButtonIndices.end();
    if (pressed && !pressedButtonIndexExists) {
        pressedButtonIndices.insert(hardwareButtonIndex);
        profile->buttonPressed(buttonDef, xplm_CommandBegin);
    } else if (pressed && pressedButtonIndexExists) {
        profile->buttonPressed(buttonDef, xplm_CommandContinue);
    } else if (!pressed && pressedButtonIndexExists) {
        pressedButtonIndices.erase(hardwareButtonIndex);
        profile->buttonPressed(buttonDef, xplm_CommandEnd);
    }
}

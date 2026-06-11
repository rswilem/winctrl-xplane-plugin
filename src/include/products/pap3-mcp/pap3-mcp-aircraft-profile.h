#ifndef PAP3MCP_AIRCRAFT_PROFILE_H
#define PAP3MCP_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <XPLMUtilities.h>

enum class PAP3MCPLed : unsigned char {
    // Dimming channels (use sendDimming with brightness 0-255)
    BACKLIGHT = 0,              // Channel 0: Panel backlight dimming
    LCD_BACKLIGHT = 1,          // Channel 1: LCD screen dimming
    OVERALL_LED_BRIGHTNESS = 2, // Channel 2: Overall brightness

    // Individual LEDs (use sendLed with ON/OFF only)
    _START = 3,
    N1 = 3,
    SPEED = 4,
    VNAV = 5,
    LVL_CHG = 6,
    HDG_SEL = 7,
    LNAV = 8,
    VORLOC = 9,
    APP = 10,
    ALT_HLD = 11,
    VS = 12,
    CMD_A = 13,
    CWS_A = 14,
    CMD_B = 15,
    CWS_B = 16,
    AT_ARM = 17,
    MA_CAPT = 18,
    MA_FO = 19,
    _END = 19,
};

enum PAP3MCPDatarefType : unsigned char {
    EXECUTE_CMD_ONCE = 1,
    EXECUTE_CMD_PHASED,
    SET_VALUE,
    SET_VALUE_PHASED,
    TOGGLE_VALUE,
    ADJUST_VALUE,
};

struct PAP3MCPButtonDef {
        std::string name;
        std::string dataref;
        PAP3MCPDatarefType datarefType = PAP3MCPDatarefType::EXECUTE_CMD_ONCE;
        double value = 0.0;
};

struct PAP3MCPDisplayData {
        float speed = 0.0f;
        int heading = 0;
        int altitude = 0;
        float verticalSpeed = 0.0f;
        bool verticalSpeedVisible = true;
        bool speedVisible = true;
        bool headingVisible = true;
        int crsCapt = 0;
        int crsFo = 0;
        bool showCourse = true;

        bool displayEnabled = true;
        bool displayTest = false; // Display test mode (all segments lit)

        bool showLabels = false;
        bool showDashesWhenInactive = false; // Show dashes (---) when displays are inactive
        bool showLabelsWhenInactive = false; // Show labels even when displays are inactive

        uint8_t machDigits = 2; // Number of MACH decimal digits to display (2 = .84, 3 = .842)

        // Special display flags
        bool digitA = false; // Special 'A' digit for SPD/MACH mode
        bool digitB = false; // Special '8' digit for bank angle

        bool ledN1 = false;
        bool ledSpeed = false;
        bool ledVNAV = false;
        bool ledLvlChg = false;
        bool ledHdgSel = false;
        bool ledLNAV = false;
        bool ledVORLOC = false;
        bool ledAPP = false;
        bool ledAltHld = false;
        bool ledVS = false;
        bool ledCmdA = false;
        bool ledCwsA = false;
        bool ledCmdB = false;
        bool ledCwsB = false;
        bool ledATArm = false;
        bool ledMaCapt = false;
        bool ledMaFO = false;
};

struct PAP3MCPEncoderDef {
        int id;
        std::string name;
        std::string incCmd;
        std::string decCmd;
};

class ProductPAP3MCP;

class PAP3MCPAircraftProfile {
    protected:
        ProductPAP3MCP *product;

    public:
        PAP3MCPAircraftProfile(ProductPAP3MCP *product) :
            product(product) {};
        virtual ~PAP3MCPAircraftProfile() {
            cleanupProfile(this);
        }

        virtual const std::vector<std::string> &displayDatarefs() const = 0;

        virtual const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const = 0;

        virtual const std::vector<PAP3MCPEncoderDef> &encoderDefs() const = 0;

        virtual void updateDisplayData(PAP3MCPDisplayData &displayData) = 0;

        virtual void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) = 0;

        // Bank angle switch handler - default does nothing, Zibo profile overrides
        virtual void handleBankAngleSwitch(uint8_t switchByte) {
            (void) switchByte;
        }

        // Maintained switch handler - default does nothing, Zibo profile overrides
        virtual void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) {
            (void) byteOffset;
            (void) bitMask;
            (void) state;
        }
};

#endif

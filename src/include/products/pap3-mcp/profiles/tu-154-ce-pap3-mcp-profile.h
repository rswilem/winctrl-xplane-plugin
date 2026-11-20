#ifndef TU_154_CE_PAP3MCP_PROFILE_H
#define TU_154_CE_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Felis Tu-154B2 Community Edition: its ABSU-154 autopilot on a 737-style MCP
class Tu154CEPAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        // LEDs are derived from several datarefs, so they are recomputed together
        // and only written when they change
        std::unordered_map<PAP3MCPLed, bool> ledStates;

        // A spring-loaded ABSU control, which the aircraft moves on for as long as it is held at 1
        // or -1. Each knob detent holds it for holdTime, and further detents keep it held.
        struct HeldSwitch {
                const char *dataref = nullptr;
                float holdTime = 0.0f;
        };

        HeldSwitch speedChange;
        HeldSwitch pitchWheel;

        void holdSwitch(HeldSwitch &held, int direction);
        // Lets go of the HeldSwitch passed as refcon once its knob stops
        static float releaseSwitch(float, float, int, void *refcon);

        void updateBrightness();
        void updateLeds();
        void setChannels(bool on);

    public:
        Tu154CEPAP3MCPProfile(ProductPAP3MCP *product);
        ~Tu154CEPAP3MCPProfile();

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
};

#endif

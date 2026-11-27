#ifndef DWSIM_ATR72_PAP3MCP_PROFILE_H
#define DWSIM_ATR72_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

// DeltaWing Simulations ATR 72-600: its FGCP and the ICP speed target on a 737-style MCP
class DWSimATR72PAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        // LEDs are derived from several datarefs, so they are recomputed together
        // and only written when they change
        std::unordered_map<PAP3MCPLed, bool> ledStates;

        void updateBrightness();
        void updateLeds();

    public:
        DWSimATR72PAP3MCPProfile(ProductPAP3MCP *product);

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

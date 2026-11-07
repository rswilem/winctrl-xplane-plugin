#ifndef AEROBASK_PHENOM_PAP3MCP_PROFILE_H
#define AEROBASK_PHENOM_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

class AerobaskPhenomPAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        // LEDs are derived from several datarefs, so they are recomputed together
        // and only written when they change
        std::unordered_map<PAP3MCPLed, bool> ledStates;
        // -1 until the bank angle switch first reports its position
        int hwLowBank = -1;

        void updateBrightness();
        void updateLeds();

    public:
        AerobaskPhenomPAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
        void handleBankAngleSwitch(uint8_t switchByte) override;
};

#endif

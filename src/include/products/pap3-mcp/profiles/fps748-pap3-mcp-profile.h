#ifndef FPS748_PAP3MCP_PROFILE_H
#define FPS748_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>

class FPS748PAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        static bool IsSSGVersion();
        void maybeToggle(const char *stateDataref, bool hwState, const char *toggleCmd);

        bool hwFDCaptOn = false;
        bool hwFDFoOn = false;
        bool hwATOn = false;
        bool hwApDiscEngaged = false;

    public:
        FPS748PAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleBankAngleSwitch(uint8_t switchByte) override;
        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
};

#endif

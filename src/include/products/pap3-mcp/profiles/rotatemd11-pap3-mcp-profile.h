#ifndef ROTATEMD11_PAP3MCP_PROFILE_H
#define ROTATEMD11_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <map>
#include <string>
#include <vector>

class RotateMD11PAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        bool backlightInitialized = false;
        bool hdgTrkSelInitialized = false;

    public:
        RotateMD11PAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();

        // Override base class methods
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;
};

#endif

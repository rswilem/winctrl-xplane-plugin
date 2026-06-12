#ifndef XCRAFTS_ERJ_PAP3_MCP_PROFILE_H
#define XCRAFTS_ERJ_PAP3_MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

class XCraftsErjPAP3MCPProfile : public PAP3MCPAircraftProfile {
    public:
        XCraftsErjPAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;
};

#endif // XCRAFTS_ERJ_PAP3_MCP_PROFILE_H

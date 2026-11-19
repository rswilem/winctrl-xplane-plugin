#ifndef TOLISS_PAP3MCP_PROFILE_H
#define TOLISS_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <vector>

class TolissPAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        bool isAnnunTest();
        void setLed(PAP3MCPLed led, bool on);
        void syncFlightDirector(const char *dataref, bool hwState, const char *toggleCmd);
        void setAutothrust(bool armed);

    public:
        TolissPAP3MCPProfile(ProductPAP3MCP *product);

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

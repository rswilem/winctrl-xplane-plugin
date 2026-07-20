#ifndef AEROBASK_PHENOM_PAP3MCP_PROFILE_H
#define AEROBASK_PHENOM_PAP3MCP_PROFILE_H

#include "g1000-pap3-mcp-profile.h"

class AerobaskPhenomPAP3MCPProfile : public G1000PAP3MCPProfile {
    private:
        // -1 until the bank angle switch first reports its position
        int hwLowBank = -1;

    public:
        AerobaskPhenomPAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();

        void handleBankAngleSwitch(uint8_t switchByte) override;
};

#endif

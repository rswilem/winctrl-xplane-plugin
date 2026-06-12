#ifndef FF777_RMP_PROFILE_H
#define FF777_RMP_PROFILE_H

#include "product-rmp.h"
#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

class FF777RMPProfile : public RMPAircraftProfile {
    private:
        std::vector<std::string> _displayDatarefs;
        std::unordered_map<RMPLed, std::string> _ledDatarefs;

        const char *rmpName() const;
        const char *sideName() const;
        const char *swapCommand() const;

    public:
        FF777RMPProfile(ProductRMP *product);
        ~FF777RMPProfile();

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

#ifndef TOLISS_RMP_PROFILE_H
#define TOLISS_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

class TolissRMPProfile : public RMPAircraftProfile {
    private:
        std::vector<std::string> _displayDatarefs;
        const char *rmpName() const;
        const char *sideName() const;
        const char *swapCommand() const;

    public:
        TolissRMPProfile(ProductRMP *product);
        ~TolissRMPProfile();

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

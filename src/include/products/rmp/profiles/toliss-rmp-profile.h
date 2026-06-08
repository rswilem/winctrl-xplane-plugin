#ifndef TOLISS_RMP_PROFILE_H
#define TOLISS_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>

class TolissRMPProfile : public RMPAircraftProfile {
    private:
        const char *rmpName() const;
        const char *sideName() const;
        static std::string formatFrequency(int hz);

    public:
        TolissRMPProfile(ProductRMP *product);
        ~TolissRMPProfile();

        static bool IsEligible();
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

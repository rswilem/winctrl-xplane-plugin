#ifndef TOLISS_TCAS_PROFILE_H
#define TOLISS_TCAS_PROFILE_H

#include "tcas-aircraft-profile.h"

#include <string>

class TolissTCASProfile : public TCASAircraftProfile {
    private:
        bool isAnnunTest();

    public:
        TolissTCASProfile(ProductTCAS *product);
        ~TolissTCASProfile();

        static bool IsEligible();
        const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const override;

        void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

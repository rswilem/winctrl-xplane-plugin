#ifndef TOLISS_ECAM_PROFILE_H
#define TOLISS_ECAM_PROFILE_H

#include "ecam-aircraft-profile.h"

#include <string>

class TolissECAMProfile : public ECAMAircraftProfile {
    public:
        TolissECAMProfile(ProductECAM *product);

        static bool IsEligible();
        const std::unordered_map<uint16_t, ECAMButtonDef> &buttonDefs() const override;

        void buttonPressed(const ECAMButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

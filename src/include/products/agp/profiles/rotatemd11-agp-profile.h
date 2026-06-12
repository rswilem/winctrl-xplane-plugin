#ifndef ROTATEMD11_AGP_PROFILE_H
#define ROTATEMD11_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

class RotateMD11AGPProfile : public AGPAircraftProfile {
    public:
        RotateMD11AGPProfile(ProductAGP *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;

        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

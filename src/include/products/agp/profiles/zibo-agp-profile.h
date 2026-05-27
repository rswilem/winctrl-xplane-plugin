#ifndef ZIBO_AGP_PROFILE_H
#define ZIBO_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

#include <string>

class ZiboAGPProfile : public AGPAircraftProfile {
    public:
        ZiboAGPProfile(ProductAGP *product);
        ~ZiboAGPProfile();

        static bool IsEligible();
        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;

        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

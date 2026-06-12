#ifndef TOLISS_AGP_PROFILE_H
#define TOLISS_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

#include <string>

class TolissAGPProfile : public AGPAircraftProfile {
    private:
        bool isAnnunTest(bool allowEssentialBusPowerOnly = false);

    public:
        TolissAGPProfile(ProductAGP *product);

        static bool IsEligible();
        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;

        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

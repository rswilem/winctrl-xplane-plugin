#ifndef FPS_EJETS_AGP_PROFILE_H
#define FPS_EJETS_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

#include <string>

class FPSEjetsAGPProfile : public AGPAircraftProfile {
    private:
        bool showDate = false;
        bool hasElapsedTime = false;

        bool hasPower();
        void updateBacklight();
        void updateLights();
        bool terrainSelected();

    public:
        FPSEjetsAGPProfile(ProductAGP *product);

        static bool IsEligible();
        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;

        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

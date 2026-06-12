#ifndef XCRAFTS_EJETS_AGP_PROFILE_H
#define XCRAFTS_EJETS_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

#include <string>

class XCraftsEjetsAGPProfile : public AGPAircraftProfile {
    public:
        XCraftsEjetsAGPProfile(ProductAGP *product);

        static bool IsEligible();
        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;

        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

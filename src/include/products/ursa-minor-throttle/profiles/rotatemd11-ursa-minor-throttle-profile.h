#ifndef ROTATEMD11_URSA_MINOR_THROTTLE_PROFILE_H
#define ROTATEMD11_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

class RotateMD11UrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    public:
        RotateMD11UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

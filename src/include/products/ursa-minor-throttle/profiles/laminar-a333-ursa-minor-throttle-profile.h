#ifndef LAMINAR_A333_URSA_MINOR_THROTTLE_PROFILE_H
#define LAMINAR_A333_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>

class LaminarA333UrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        bool isAnnunTest();
        std::string trimText;

    public:
        LaminarA333UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

#ifndef ZIBO_URSA_MINOR_THROTTLE_PROFILE_H
#define ZIBO_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>

class ZiboUrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        std::string trimText;

    public:
        ZiboUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);
        ~ZiboUrsaMinorThrottleProfile();

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

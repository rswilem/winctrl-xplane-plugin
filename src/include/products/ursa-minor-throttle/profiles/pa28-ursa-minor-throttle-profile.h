#ifndef PA28_URSA_MINOR_THROTTLE_PROFILE_H
#define PA28_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>
#include <unordered_map>

class PA28UrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

    public:
        PA28UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

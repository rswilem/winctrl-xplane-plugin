#ifndef XCRAFTS_ERJ_URSA_MINOR_THROTTLE_PROFILE_H
#define XCRAFTS_ERJ_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>

class XCraftsErjUrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        std::string trimText;

    public:
        XCraftsErjUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

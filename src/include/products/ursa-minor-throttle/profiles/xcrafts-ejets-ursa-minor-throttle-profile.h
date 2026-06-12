#ifndef XCRAFTS_URSA_MINOR_THROTTLE_PROFILE_H
#define XCRAFTS_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>

class XCraftsEjetsUrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        std::string trimText;

    public:
        XCraftsEjetsUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

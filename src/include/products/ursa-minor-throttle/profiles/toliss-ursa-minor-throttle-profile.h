#ifndef TOLISS_URSA_MINOR_THROTTLE_PROFILE_H
#define TOLISS_URSA_MINOR_THROTTLE_PROFILE_H

#include "ursa-minor-throttle-aircraft-profile.h"

#include <string>

class TolissUrsaMinorThrottleProfile : public UrsaMinorThrottleAircraftProfile {
    private:
        bool isAnnunTest();
        std::string trimText;

    public:
        TolissUrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product);
        ~TolissUrsaMinorThrottleProfile();

        static bool IsEligible();

        const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const override;
        void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

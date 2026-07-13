#ifndef PA28_ECAM_PROFILE_H
#define PA28_ECAM_PROFILE_H

#include "ecam-aircraft-profile.h"

#include <string>
#include <unordered_map>

class PA28ECAMProfile : public ECAMAircraftProfile {
    private:
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

    public:
        PA28ECAMProfile(ProductECAM *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, ECAMButtonDef> &buttonDefs() const override;
        void buttonPressed(const ECAMButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

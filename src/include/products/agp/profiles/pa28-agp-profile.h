#ifndef PA28_AGP_PROFILE_H
#define PA28_AGP_PROFILE_H

#include "agp-aircraft-profile.h"

#include <string>
#include <unordered_map>

class PA28AGPProfile : public AGPAircraftProfile {
    private:
        bool etRunning = true;
        bool showDate = false;
        double etAccumulatedSec = 0.0;
        double etLastFlightTime = -1.0;
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

    public:
        PA28AGPProfile(ProductAGP *product);

        static bool IsEligible();

        const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const override;
        void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

#ifndef FPS_EJETS_PDC_PROFILE_H
#define FPS_EJETS_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class FPSEjetsPDCProfile : public PDCAircraftProfile {
    private:
        char minimumsSlowDelta = 0;
        char minimumsFastDelta = 0;
        float minimumsLastCommandTime = 0.0f;
        float minimumsFastReleaseTime = 0.0f;

        char baroSlowDelta = 0;
        char baroFastDelta = 0;
        float baroLastCommandTime = 0.0f;
        float baroFastReleaseTime = 0.0f;

        int lastRangeDetent = -1;

        std::string sideDataref(const char *pilot, const char *copilot) const;
        void handleKnob(const PDCButtonDef *button, XPLMCommandPhase phase);
        void changeMinimums(char delta);
        void changeBaro(char delta);
        void selectRangeDetent(int detent);

    public:
        FPSEjetsPDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

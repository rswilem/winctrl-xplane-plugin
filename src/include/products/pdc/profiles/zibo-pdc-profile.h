#ifndef ZIBO_PDC_PROFILE_H
#define ZIBO_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class ZiboPDCProfile : public PDCAircraftProfile {
    private:
        // Slow and fast detents are tracked separately: the second detent also
        // closes (or passes through) the slow contact, so a single shared delta
        // would let the slow ±1 overwrite the fast ±10 (issue #109).
        char minimumsSlowDelta = 0;
        char minimumsFastDelta = 0;
        float minimumsFastReleaseTime = -1.0f;
        float minimumsLastCommandTime = 0.0f;

        char baroSlowDelta = 0;
        char baroFastDelta = 0;
        float baroFastReleaseTime = -1.0f;
        float baroLastCommandTime = 0.0f;

        void changeMinimums(char delta);
        void changeBaro(char delta);

    public:
        ZiboPDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

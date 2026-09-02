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

        // Zibo parks the minimums at a negative sentinel when unset (RST, or
        // never set), so the knob resumes from the last value the pilot dialled
        // in; 200/100 are only the initial defaults per mode (issue #109).
        float lastMinimumsBaro = 200.0f;
        float lastMinimumsRadio = 100.0f;

        // Set when STD engages: the next knob turn starts from 1013/29.92, but
        // nothing is written until then so no preselect is shown (issue #109).
        bool baroStdSeedPending = false;

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

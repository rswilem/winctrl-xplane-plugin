#ifndef G1000_PDC_PROFILE_H
#define G1000_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>
#include <unordered_map>

// Shared PDC support for aircraft with X-Plane's G1000. Aircraft profiles derive from it and
// add their own eligibility check.
class G1000PDCProfile : public PDCAircraftProfile {
    private:
        // The fast detent also closes the slow contact, so both are tracked
        // separately (same reasoning as the Zibo profile, issue #109).
        struct KnobState {
                float fastRepeatInterval;
                // Slow and fast turn different rings, so a slow step waits until
                // it is clear the knob is not on its way to the fast detent
                bool deferSlow;

                const PDCButtonDef *slow = nullptr;
                const PDCButtonDef *fast = nullptr;
                // Deferred slow step, fired once the fast contact had its chance to close
                const PDCButtonDef *pendingSlow = nullptr;
                bool fastSeen = false;
                bool slowFired = false;
                float lastCommandTime = 0.0f;
                float pendingSlowTime = 0.0f;
                float fastReleaseTime = -1.0f;
        };

        // The range controls work the MFD map from both sides instead of the captain's PFD inset
        const bool rangeOnMfd;
        // Built once per device variant, on first use
        mutable std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> buttonCache;

        KnobState baroKnob = {.fastRepeatInterval = 0.03f, .deferSlow = false};
        KnobState fmsKnob = {.fastRepeatInterval = 0.25f, .deferSlow = true};
        int lastRangePosition = -1;

        void updateBacklight();
        void knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase);
        void stepRange(const PDCButtonDef *button);

    public:
        G1000PDCProfile(ProductPDC *product, bool rangeOnMfd = false);

        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

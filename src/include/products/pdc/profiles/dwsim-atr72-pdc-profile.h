#ifndef DWSIM_ATR72_PDC_PROFILE_H
#define DWSIM_ATR72_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>
#include <unordered_map>

// DeltaWing Simulations ATR 72-600: the own-side EFCP (ND range, format, bearings, MFD pages) and
// ICP (baro, DH / MDA) on a PDC
class DWSimATR72PDCProfile : public PDCAircraftProfile {
    private:
        // The fast detent also closes the slow contact, so both are tracked
        // separately (same reasoning as the Zibo profile, issue #109).
        struct KnobState {
                const PDCButtonDef *slow = nullptr;
                const PDCButtonDef *fast = nullptr;
                float fastReleaseTime = -1.0f;
                float nextStepTime = 0.0f;
        };

        // Built once per device variant, on first use
        mutable std::unordered_map<PDCDeviceVariant, std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef>> buttonCache;

        KnobState baroKnob;
        KnobState minimumsKnob;
        // The map mode selector's last position, -1 until it reports one
        int lastFormatPosition = -1;
        // The range the 3N selector last asked for, -1 until it does, and when
        int requestedRange = -1;
        float requestedRangeTime = 0.0f;
        // The VOR/ADF switches' last positions in the BRG cycle, -1 until they report one
        int lastBearing1 = -1;
        int lastBearing2 = -1;

        std::string efcp() const;
        std::string icp() const;
        std::string minimums() const;

        void knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase);
        void selectRange(int range);
        void selectFormat(int position);
        void selectMinimums(int reference);
        void selectBearing(const PDCButtonDef *button);
        void updateBacklight();

    public:
        DWSimATR72PDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

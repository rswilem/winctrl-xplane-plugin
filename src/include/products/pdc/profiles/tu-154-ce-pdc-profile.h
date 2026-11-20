#ifndef TU_154_CE_PDC_PROFILE_H
#define TU_154_CE_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>
#include <unordered_map>

// Felis Tu-154B2 Community Edition: a pilot's VBE and RV-5 altimeters, the UVID and the GNS 430's
// map range on a PDC
class Tu154CEPDCProfile : public PDCAircraftProfile {
    private:
        // The fast detent also closes the slow contact, so both are tracked
        // separately (same reasoning as the Zibo profile, issue #109).
        struct KnobState {
                const PDCButtonDef *slow = nullptr;
                const PDCButtonDef *fast = nullptr;
                float fastReleaseTime = -1.0f;
                float nextStepTime = 0.0f;
        };

        enum class Altimeter {
            VBE,
            UVID,
        };

        enum class Minimums {
            // The RV-5's
            DecisionHeight,
            // The VBE's
            AlertAltitude,
        };

        // The PDC's own side, "left" or "right" as named in the altimeter datarefs
        const std::string side;
        // The altimeter the BARO knob sets, picked by the IN/HPA switch
        Altimeter altimeter = Altimeter::VBE;
        // What the MINS knob sets, picked by the RADIO/BARO switch
        Minimums minimums = Minimums::AlertAltitude;
        KnobState baroKnob;
        KnobState minimumsKnob;
        // The 3N range selector's last position, -1 until it reports one
        int lastRangePosition = -1;
        // The dataref each held push set, released even if a switch moved meanwhile
        std::unordered_map<const PDCButtonDef *, std::string> heldPushes;

        // "t154/vbe/vbe_l_" or "t154/vbe/vbe_r_", the start of the VBE knob commands
        std::string vbeCommands() const;
        void push(const PDCButtonDef *button, XPLMCommandPhase phase);
        bool hasGns() const;
        void updateBacklight();
        void knobPressed(KnobState &knob, const PDCButtonDef *button, XPLMCommandPhase phase);
        void turnKnob(const PDCButtonDef *button);
        void selectRange(int position);

    public:
        Tu154CEPDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

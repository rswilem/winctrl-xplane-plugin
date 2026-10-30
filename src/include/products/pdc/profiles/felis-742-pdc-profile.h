#ifndef FELIS_742_PDC_PROFILE_H
#define FELIS_742_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class Felis742PDCProfile : public PDCAircraftProfile {
    private:
        // Positions in the Felis ND panel datarefs

        enum class NDButton {
            Data = 0,
            Waypoint = 1,
            Navaid = 2,
            Airport = 3,
            Weather = 4,
            Heading = 5,
        };

        enum class NDMode {
            Map = 0,
            Plan = 1,
            INS = 2,
        };

        enum class NDRange {
            Nm10 = 0,
            Nm20 = 1,
            Nm40 = 2,
            Nm80 = 3,
            Nm160 = 4,
            Nm320 = 5,
            Nm640 = 6,
        };

        // The fast detent also closes (or passes through) the slow contact,
        // so both are tracked separately (same reasoning as the Zibo profile, issue #109).
        struct BaroKnobState {
                const PDCButtonDef *slow = nullptr;
                const PDCButtonDef *fast = nullptr;
                float lastCommandTime = 0.0f;
                float fastReleaseTime = -1.0f;
        };

        BaroKnobState baroKnob;
        // Mins knob positions currently closed; the DH command is held while any is
        int minsKnobPositionsHeld = 0;
        std::string minsCommand;

        void baroKnobPressed(const PDCButtonDef *button, XPLMCommandPhase phase);
        void minsKnobPressed(const PDCButtonDef *button, XPLMCommandPhase phase);
        void pressNDButton(NDButton button, bool pressed);
        void stepNDRange(int delta);

    public:
        Felis742PDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

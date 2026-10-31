#ifndef FELIS_742_RMP_PROFILE_H
#define FELIS_742_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

class Felis742RMPProfile : public RMPAircraftProfile {
    private:
        enum class Radio {
            VHF1,
            VHF2,
            VHF3,
            NAV1,
            NAV2,
            ADF1,
            ADF2,
        };

        enum class Knob {
            LargeDown,
            LargeUp,
            SmallDown,
            SmallUp,
        };

        // The radio this RMP drives
        Radio selectedRadio = Radio::VHF1;
        // The ADF has a 1 kHz knob as well; the knob push moves the small knob between 10 kHz and 1 kHz
        bool adfFineTuning = false;

        bool isNavRadio() const;
        bool isADFRadio() const;
        // VHF 1-3, NAV 1-2 or ADF 1-2, as numbered in the cockpit datarefs and commands
        int radioNumber() const;
        // The selected panel's transfer switch, 0 with the left window active and 1 with the right
        std::string transferSwitch() const;
        bool isLeftWindowActive() const;
        bool isRadioPowered() const;
        void selectRadio(Radio radio);
        void throwTransferSwitch();
        void turnKnob(Knob knob);
        void updateLeds();

    public:
        Felis742RMPProfile(ProductRMP *product);

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

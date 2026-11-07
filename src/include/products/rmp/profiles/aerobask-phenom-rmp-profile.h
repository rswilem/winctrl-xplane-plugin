#ifndef AEROBASK_PHENOM_RMP_PROFILE_H
#define AEROBASK_PHENOM_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

class AerobaskPhenomRMPProfile : public RMPAircraftProfile {
    private:
        enum class Radio {
            COM1,
            COM2,
            NAV1,
            NAV2,
            ADF,
        };

        enum class Knob {
            LargeDown,
            LargeUp,
            SmallDown,
            SmallUp,
        };

        // The radio this RMP drives
        Radio selectedRadio = Radio::COM1;
        // The ADF has a 1 kHz knob as well; the knob push moves the small knob between 10 kHz and 1 kHz
        bool adfFineTuning = false;

        // "com1", "nav2", "adf1" and so on, as named in the radio datarefs and commands
        std::string radioName() const;
        bool isRadioPowered() const;
        void selectRadio(Radio radio);
        void turnKnob(Knob knob);
        void updateBacklight();
        void updateLeds();

    public:
        AerobaskPhenomRMPProfile(ProductRMP *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

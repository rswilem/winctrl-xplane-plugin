#ifndef TU_154_CE_RMP_PROFILE_H
#define TU_154_CE_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

// Felis Tu-154B2 Community Edition: its VHF and NAV radios on an RMP
class Tu154CERMPProfile : public RMPAircraftProfile {
    private:
        enum class Radio {
            VHF1,
            VHF2,
            VHF3,
            NAV1,
            NAV2,
        };

        enum class Knob {
            LargeDown,
            LargeUp,
            SmallDown,
            SmallUp,
        };

        // The radio this RMP tunes
        Radio selectedRadio = Radio::VHF1;

        void selectRadio(Radio radio);
        void turnKnob(Knob knob);
        std::string frequencyText() const;
        bool isRadioPowered() const;
        void updateBacklight();
        void updateLeds();

    public:
        Tu154CERMPProfile(ProductRMP *product);

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

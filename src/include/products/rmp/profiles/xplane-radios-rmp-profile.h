#ifndef XPLANE_RADIOS_RMP_PROFILE_H
#define XPLANE_RADIOS_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <vector>

// Shared RMP support for aircraft whose radios are X-Plane's own, such as those of the G1000 and
// the GNS 530/430. Aircraft profiles derive from it and add their own eligibility check.
class XPlaneRadiosRMPProfile : public RMPAircraftProfile {
    public:
        // The panel lighting the backlight follows: one element of a lighting dataref
        struct Backlight {
                const char *dataref;
                int index;
        };

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

        Backlight backlight;
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
        // X-Plane's generic panel lighting unless the aircraft lights its radios from elsewhere
        XPlaneRadiosRMPProfile(ProductRMP *product, Backlight backlight = {"sim/cockpit2/electrical/panel_brightness_ratio", 0});

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;

        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;

        void updateDisplays() override;
};

#endif

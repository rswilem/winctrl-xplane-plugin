#ifndef FF350_FMC_PROFILE_H
#define FF350_FMC_PROFILE_H

#include "toliss-fmc-profile.h"

// The FlightFactor A350 v1 exposes the same AirbusFBW/MCDU* display and key
// datarefs as the ToLiss A3xx, so it reuses the ToLiss rendering, button
// mapping and colour handling wholesale. It differs only in brightness: the
// MCDU screen backlight is driven by a dedicated pedestal-knob dataref
// (1-sim/lights/mcdu/Rotery) rather than AirbusFBW/DUBrightness, and it has no
// AirbusFBW self-test animation on the panel. The constructor rewires those two
// bits and buttonPressed() retargets the BRIGHT/DIM keys onto the knob dataref.
class FF350FMCProfile : public TolissFMCProfile {
    public:
        FF350FMCProfile(ProductFMC *product);

        static bool IsEligible();
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

#ifndef ROTATEMD11_TCAS_PROFILE_H
#define ROTATEMD11_TCAS_PROFILE_H

#include "tcas-aircraft-profile.h"

class RotateMD11TCASProfile : public TCASAircraftProfile {
    public:
        RotateMD11TCASProfile(ProductTCAS *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const override;

        void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

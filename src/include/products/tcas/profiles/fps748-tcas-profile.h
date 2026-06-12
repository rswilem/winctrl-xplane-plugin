#ifndef FPS748_TCAS_PROFILE_H
#define FPS748_TCAS_PROFILE_H

#include "tcas-aircraft-profile.h"

#include <string>

class FPS748TCASProfile : public TCASAircraftProfile {
    private:
        static bool IsSSGVersion();

    public:
        FPS748TCASProfile(ProductTCAS *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const override;
        void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

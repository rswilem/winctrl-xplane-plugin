#ifndef PA28_TCAS_PROFILE_H
#define PA28_TCAS_PROFILE_H

#include "tcas-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

class PA28TCASProfile : public TCASAircraftProfile {
    private:
        std::string codeEntry;
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

    public:
        PA28TCASProfile(ProductTCAS *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const override;
        void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

#ifndef XCRAFTS_ERJ_TCAS_PROFILE_H
#define XCRAFTS_ERJ_TCAS_PROFILE_H

#include "tcas-aircraft-profile.h"

#include <string>
#include <XPLMUtilities.h>

class XCraftsERJTCASProfile : public TCASAircraftProfile {
    private:
        std::string squawkInput;

    public:
        XCraftsERJTCASProfile(ProductTCAS *product);
        ~XCraftsERJTCASProfile();

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const override;
        void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

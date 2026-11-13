#ifndef PA28_RMP_PROFILE_H
#define PA28_RMP_PROFILE_H

#include "rmp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

enum class PA28RMPRadio : unsigned char {
    COM1 = 0,
    COM2,
    NAV1,
    NAV2,
    ADF1,
};

class PA28RMPProfile : public RMPAircraftProfile {
    private:
        PA28RMPRadio selectedRadio = PA28RMPRadio::COM1;
        bool panelOn = true;
        bool altFineStep = false;
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

        void updateLeds();

    public:
        PA28RMPProfile(ProductRMP *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const override;
        void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) override;
        void updateDisplays() override;
};

#endif

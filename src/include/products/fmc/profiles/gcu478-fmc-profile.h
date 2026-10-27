#ifndef GCU478_FMC_PROFILE_H
#define GCU478_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

// Shared FMC support for aircraft with X-Plane's GCU 478, the G1000 keyboard and FMS knob.
// Aircraft profiles derive from it and add their own eligibility check.
class GCU478FMCProfile : public FMCAircraftProfile {
    private:
        void updateBacklight();

    public:
        GCU478FMCProfile(ProductFMC *product);

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

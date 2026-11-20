#ifndef DWSIM_ATR72_FMC_PROFILE_H
#define DWSIM_ATR72_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <string>

class DWSimATR72FMCProfile : public FMCAircraftProfile {
    private:
        bool screenExportAvailable = false;

        std::string mcdu() const;
        std::string screenRef(const char *suffix) const;
        std::string screenLineRef(const char *suffix, int line) const;
        void updateScreenBacklight();

    public:
        DWSimATR72FMCProfile(ProductFMC *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

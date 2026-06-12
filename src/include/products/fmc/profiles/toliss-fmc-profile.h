#ifndef TOLISS_FMC_PROFILE_H
#define TOLISS_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <regex>

class TolissFMCProfile : public FMCAircraftProfile {
    private:
        std::regex datarefRegex;
        bool scratchpadPaddingActive;
        bool isSelfTest;
        unsigned char selfTestDisplayHelper;

    public:
        TolissFMCProfile(ProductFMC *product);

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

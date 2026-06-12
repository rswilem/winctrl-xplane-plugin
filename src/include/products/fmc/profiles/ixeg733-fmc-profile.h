#ifndef IXEG733_FMC_PROFILE_H
#define IXEG733_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <regex>

class IXEG733FMCProfile : public FMCAircraftProfile {
    private:
        std::pair<std::string, std::vector<char>> processIxegText(const std::vector<unsigned char> &characters);

    public:
        IXEG733FMCProfile(ProductFMC *product);

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

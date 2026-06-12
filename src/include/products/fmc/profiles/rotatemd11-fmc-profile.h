#ifndef ROTATEMD11_FMC_PROFILE_H
#define ROTATEMD11_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

class RotateMD11FMCProfile : public FMCAircraftProfile {
    private:
        std::string processUTF8Arrows(const std::string &input);

    public:
        RotateMD11FMCProfile(ProductFMC *product);

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;
        bool shouldReadDatarefAsBytes(const std::string &dataref) const override;
};

#endif

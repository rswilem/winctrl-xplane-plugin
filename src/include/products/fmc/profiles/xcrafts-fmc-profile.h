#ifndef XCRAFTS_FMC_PROFILE_H
#define XCRAFTS_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <regex>

enum class XCraftsFMCFontStyle : unsigned char {
    Large = 1,
    Small = 2,
    LargeReverseVideo = 3,
    SmallReverseVideo = 4,
    LargeReverseVideoBoxed = 5,
    SmallReverseVideoBoxed = 6
};

class XCraftsFMCProfile : public FMCAircraftProfile {
    private:
        std::regex datarefRegex;

    public:
        XCraftsFMCProfile(ProductFMC *product);
        virtual ~XCraftsFMCProfile();

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

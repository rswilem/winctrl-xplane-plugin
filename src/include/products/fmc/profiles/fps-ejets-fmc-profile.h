#ifndef FPS_EJETS_FMC_PROFILE_H
#define FPS_EJETS_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <regex>
#include <string>
#include <vector>

class FPSEjetsFMCProfile : public FMCAircraftProfile {
    private:
        std::regex datarefRegex;
        std::vector<std::string> lineDatarefs;
        std::vector<FMCButtonDef> buttons;
        std::unordered_map<FMCKey, const FMCButtonDef *> keyMap;
        static std::string UfmcPrefix();
        std::string BrightnessDataref() const;
        void updateBacklight();

    public:
        FPSEjetsFMCProfile(ProductFMC *product);

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

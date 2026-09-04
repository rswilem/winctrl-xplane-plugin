#ifndef FFA320_FMC_PROFILE_H
#define FFA320_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <array>
#include <string>

// FlightFactor A320 ultimate (X-Plane 11 only, package 1.3.6).
//
// The aircraft registers X-Plane datarefs and commands for its cockpit click
// objects (a320/MCDU1/Key1 and friends) but nothing for the screen: the display
// lives in FlightFactor's own value tree, reachable only through the shared
// value interface in ffsharedvalues.h. So both the display and the keys are
// driven through that interface, mirroring what the aircraft's own web MCDU
// (data/HTML/JS/mcdu.js) does, which also avoids guessing how the flat
// a320/MCDU1/Line1..12 datarefs map onto left and right line keys.
//
// Display: DisplayLines1..14 hold 24 characters per row, DisplayAttrs1..14 one
// attribute character per cell. The attribute letter is the colour and its case
// is the font size, uppercase being large. Row 14 is the scratchpad.
class FFA320FMCProfile : public FMCAircraftProfile {
    private:
        static constexpr unsigned int LineCount = 14;
        static constexpr unsigned int CharsPerLine = 24;

        // "Aircraft.FMGS.MCDU1" / "Aircraft.Cockpit.MCDU1" for the captain side,
        // MCDU2 for the first officer.
        std::string fmgsPath;
        std::string cockpitPath;
        std::string commandPrefix;
        std::string displayVersionRef;

        std::array<std::string, FFA320FMCProfile::LineCount> linePaths;
        std::array<std::string, FFA320FMCProfile::LineCount> attrPaths;
        std::array<std::string, FFA320FMCProfile::LineCount> lines;
        std::array<std::string, FFA320FMCProfile::LineCount> attrs;

        // Bumped whenever the snapshot changes; published as a dataref so the
        // product's existing display change detection has something to watch.
        int displayVersion = 0;
        bool retained = false;
        bool loggedBrightness = false;
        bool loggedMissingDisplay = false;

        void connectToAircraft();
        void pollAircraft();
        void updateAnnunciators();
        void updateBrightness();

    public:
        FFA320FMCProfile(ProductFMC *product);
        ~FFA320FMCProfile();

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

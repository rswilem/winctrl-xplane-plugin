#ifndef AEROBASK_PHENOM_FMC_PROFILE_H
#define AEROBASK_PHENOM_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <string>
#include <vector>
#include <XPLMNavigation.h>

class AerobaskPhenomFMCProfile : public FMCAircraftProfile {
    private:
        using CountEntriesFn = int (*)(int);
        using DestinationEntryFn = int (*)(int);
        using EntryInfoFn = void (*)(int, int, XPLMNavType *, char *, XPLMNavRef *, int *, float *, float *);

        struct Waypoint {
                std::string id;
                XPLMNavType type = xplm_Nav_Unknown;
                int altitude = 0;
                float latitude = 0.0f;
                float longitude = 0.0f;
        };

        struct FlightPlan {
                std::vector<Waypoint> waypoints;
                int activeIndex = -1;
        };

        struct Row {
                std::string text;
                char color = 'w';
                bool fontSmall = false;
        };

        struct Legs {
                std::vector<Row> rows;
                int activeRow = 0;

                int pageCount() const;
        };

        CountEntriesFn countEntries = nullptr;
        DestinationEntryFn destinationEntry = nullptr;
        EntryInfoFn entryInfo = nullptr;

        std::vector<std::vector<char>> drawnPage;
        int scrolledPage = -1;
        int shownPage = 0;
        int lastActiveRow = -1;

        bool hasFlightPlan() const;
        FlightPlan readFlightPlan(int plan) const;
        void refresh();
        Legs listLegs() const;
        void choosePage(const Legs &legs);
        void draw(std::vector<std::vector<char>> &page, const Legs &legs) const;
        void scroll(int pages);
        static float pollLoop(float elapsedSinceLastCall, float elapsedSinceLastFlightLoop, int counter, void *refcon);

        void updateBacklight();

    public:
        AerobaskPhenomFMCProfile(ProductFMC *product);
        ~AerobaskPhenomFMCProfile() override;

        AerobaskPhenomFMCProfile(const AerobaskPhenomFMCProfile &) = delete;
        AerobaskPhenomFMCProfile &operator=(const AerobaskPhenomFMCProfile &) = delete;

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

#ifndef FMC_FLIGHT_PLAN_PAGE_H
#define FMC_FLIGHT_PLAN_PAGE_H

#include "fmc-aircraft-profile.h"

#include <map>
#include <string>
#include <vector>
#include <XPLMNavigation.h>
#include <XPLMUtilities.h>

class ProductFMC;

// The pilot's flight plan of X-Plane's built-in G1000 or GNS 530/430 on the FMC screen, listing each
// leg like their flight plan pages. An FMC profile opts in by owning one and passing its screen
// and the scroll keys through to it: updatePage(), colorMap() and mapCharacter() for the
// screen, scrollUpButton() and scrollDownButton() added to its buttonDefs() and buttonPressed()
// called before its own.
class FMCFlightPlanPage {
    private:
        ProductFMC *product = nullptr;

        // The page as last drawn
        std::vector<std::vector<char>> drawnPage;
        // The page scrolled to with the line select keys, or -1 to follow the active leg
        int scrolledPage = -1;
        int shownPage = 0;
        int lastActiveRow = -1;

        struct Row {
                std::string text;
                char color = 'w';
                bool fontSmall = false;
        };

        // The legs of both flight plans as page rows, and the row of the leg being flown
        struct Legs {
                std::vector<Row> rows;
                int activeRow = 0;

                int pageCount() const;
        };

        void refresh();
        Legs listLegs() const;
        // Picks the page to show, following the active leg unless scrolled away from it
        void choosePage(const Legs &legs);
        void draw(std::vector<std::vector<char>> &page, const Legs &legs) const;
        void scroll(int pages);
        static float pollLoop(float elapsedSinceLastCall, float elapsedSinceLastFlightLoop, int counter, void *refcon);

    public:
        FMCFlightPlanPage(ProductFMC *product);
        ~FMCFlightPlanPage();

        FMCFlightPlanPage(const FMCFlightPlanPage &) = delete;
        FMCFlightPlanPage &operator=(const FMCFlightPlanPage &) = delete;

        // The line select keys that scroll the page, for the profile's buttonDefs():
        // the top ones page back, the bottom ones forward
        static FMCButtonDef scrollUpButton();
        static FMCButtonDef scrollDownButton();
        // Handles the scroll keys and returns whether the button was one of them
        bool buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase);

        void updatePage(std::vector<std::vector<char>> &page) const;
        static const std::map<char, FMCTextColor> &colorMap();
        // Writes the page's special characters and returns whether the character was one of them
        static bool mapCharacter(std::vector<uint8_t> *buffer, uint8_t character);
};

#endif

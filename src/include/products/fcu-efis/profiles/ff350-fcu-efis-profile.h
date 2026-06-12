#ifndef FFA350_FCU_EFIS_PROFILE_H
#define FFA350_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <vector>

class FF350FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        bool isAnnunTest();

    public:
        FF350FCUEfisProfile(ProductFCUEfis *product);

        static bool IsEligible();

        // Override base class methods
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;

        bool hasEfisLeft() const override {
            return true;
        }

        bool hasEfisRight() const override {
            return true;
        }

        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

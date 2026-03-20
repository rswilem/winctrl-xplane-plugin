#ifndef C172_FCU_EFIS_PROFILE_H
#define C172_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <unordered_map>

class C172FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        int altitudeIncrements = 0;

    public:
        C172FCUEfisProfile(ProductFCUEfis *product);
        ~C172FCUEfisProfile();

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;

        bool hasEfisLeft() const override {
            return true;
        }

        bool hasEfisRight() const override {
            return false;
        }

        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

#ifndef PA28_FCU_EFIS_PROFILE_H
#define PA28_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <unordered_map>

class PA28FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        float qnhBeforeStd = 0.0f;
        bool stdMode = false;
        bool captainUnitIsHpa = true;
        bool foUnitIsHpa = false;
        float altitudeIncrement = 100.0f;
        int lastBacklightSent = -1;
        int lastScreensSent = -1;

    public:
        PA28FCUEfisProfile(ProductFCUEfis *product);

        static bool IsEligible();

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

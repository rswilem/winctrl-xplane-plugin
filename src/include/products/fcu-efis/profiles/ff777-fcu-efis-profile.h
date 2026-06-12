#ifndef FF777_FCU_EFIS_PROFILE_H
#define FF777_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <vector>

class FF777FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        bool isTestMode();
        bool isStdCaptain;
        bool isStdFirstOfficer;

    public:
        FF777FCUEfisProfile(ProductFCUEfis *product);

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

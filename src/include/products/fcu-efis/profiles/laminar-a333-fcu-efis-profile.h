#ifndef LAMINAR_A333_FCU_EFIS_PROFILE_H
#define LAMINAR_A333_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <vector>

class LaminarA333FCUEfisProfile : public FCUEfisAircraftProfile {
    public:
        LaminarA333FCUEfisProfile(ProductFCUEfis *product);

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

#endif // LAMINAR_A333_FCU_EFIS_PROFILE_H

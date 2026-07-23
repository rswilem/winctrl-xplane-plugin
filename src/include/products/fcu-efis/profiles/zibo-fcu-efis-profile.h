#ifndef ZIBO_FCU_EFIS_PROFILE_H
#define ZIBO_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <map>
#include <string>
#include <unordered_map>

class ZiboFCUEfisProfile : public FCUEfisAircraftProfile {
    public:
        ZiboFCUEfisProfile(ProductFCUEfis *product);

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

    private:
        // Altitude selector step set by the 100/1000 switch (buttons 25/26)
        int altitudeIncrement = 100;
};

#endif // ZIBO_FCU_EFIS_PROFILE_H

#ifndef ZIBO_PDC_PROFILE_H
#define ZIBO_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class ZiboPDCProfile : public PDCAircraftProfile {
    private:
        char minimumsDelta = 0;
        float minimumsLastCommandTime = 0.0f;

        char baroDelta = 0;
        float baroLastCommandTime = 0.0f;

        void changeMinimums();
        void changeBaro();

    public:
        ZiboPDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

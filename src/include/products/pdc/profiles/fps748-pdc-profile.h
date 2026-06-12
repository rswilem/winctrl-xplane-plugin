#ifndef FPS748_PDC_PROFILE_H
#define FPS748_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class FPS748PDCProfile : public PDCAircraftProfile {
    private:
        static bool IsSSGVersion();

        char minimumsDelta = 0;
        float minimumsLastCommandTime = 0.0f;

        char baroDelta = 0;
        float baroLastCommandTime = 0.0f;

        void changeMinimums();
        void changeBaro();

    public:
        FPS748PDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

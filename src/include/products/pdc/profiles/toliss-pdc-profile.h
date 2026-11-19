#ifndef TOLISS_PDC_PROFILE_H
#define TOLISS_PDC_PROFILE_H

#include "pdc-aircraft-profile.h"

#include <string>

class TolissPDCProfile : public PDCAircraftProfile {
    private:
        char baroSlowDelta = 0;
        char baroFastDelta = 0;
        float baroFastReleaseTime = -1.0f;
        float baroLastCommandTime = 0.0f;

        bool mapCentered = false;

        std::string sideDataref(const char *captain, const char *firstOfficer) const;
        void changeBaro(char delta);
        void handleCustomButton(const PDCButtonDef *button);

    public:
        TolissPDCProfile(ProductPDC *product);

        static bool IsEligible();
        const std::unordered_map<PDCButtonIndex3N3M, PDCButtonDef> &buttonDefs() const override;

        void update() override;
        void buttonPressed(const PDCButtonDef *button, XPLMCommandPhase phase) override;
};

#endif

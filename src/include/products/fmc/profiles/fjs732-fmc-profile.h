#ifndef FJS732_FMC_PROFILE_H
#define FJS732_FMC_PROFILE_H

#include "uns1-fmc-profile.h"

// FlyJSim 737-200: single CDU/FMS (no captain/FO split) on the stock UNS-1
// "uns1/cdu1"/"uns1/fms1" namespace, so it inherits UNS1FMCProfile's default path
// prefixes unchanged. 
class FJS732FMCProfile : public UNS1FMCProfile {
    public:
        FJS732FMCProfile(ProductFMC *product);

        static bool IsEligible();
};

#endif // FJS732_FMC_PROFILE_H

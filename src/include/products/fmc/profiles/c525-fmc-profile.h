#ifndef C525_FMC_PROFILE_H
#define C525_FMC_PROFILE_H

#include "uns1-fmc-profile.h"

// TorqueSim Citation CJ (C525): single-pilot aircraft with one CDU/FMS on the
// stock UNS-1 "uns1/cdu1"/"uns1/fms1" namespace, so it inherits UNS1FMCProfile's
// default path prefixes unchanged; only its eligibility check is aircraft-specific.
class C525FMCProfile : public UNS1FMCProfile {
    public:
        C525FMCProfile(ProductFMC *product);

        static bool IsEligible();
};

#endif // C525_FMC_PROFILE_H

#ifndef AEROBASK_PHENOM_FMC_PROFILE_H
#define AEROBASK_PHENOM_FMC_PROFILE_H

#include "gcu478-fmc-profile.h"

class AerobaskPhenomFMCProfile : public GCU478FMCProfile {
    public:
        AerobaskPhenomFMCProfile(ProductFMC *product);

        static bool IsEligible();
};

#endif

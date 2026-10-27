#ifndef AEROBASK_PHENOM_RMP_PROFILE_H
#define AEROBASK_PHENOM_RMP_PROFILE_H

#include "xplane-radios-rmp-profile.h"

class AerobaskPhenomRMPProfile : public XPlaneRadiosRMPProfile {
    public:
        AerobaskPhenomRMPProfile(ProductRMP *product);

        static bool IsEligible();
};

#endif

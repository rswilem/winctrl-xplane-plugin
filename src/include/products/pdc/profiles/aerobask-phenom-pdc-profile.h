#ifndef AEROBASK_PHENOM_PDC_PROFILE_H
#define AEROBASK_PHENOM_PDC_PROFILE_H

#include "g1000-pdc-profile.h"

class AerobaskPhenomPDCProfile : public G1000PDCProfile {
    public:
        AerobaskPhenomPDCProfile(ProductPDC *product);

        static bool IsEligible();
};

#endif

#ifndef Q4XP_FMC_PROFILE_H
#define Q4XP_FMC_PROFILE_H

#include "uns1-fmc-profile.h"

class Q4XPFMCProfile : public UNS1FMCProfile {
    public:
        Q4XPFMCProfile(ProductFMC *product);

        static bool IsEligible();

    protected:
        std::string displayPathPrefix() const override;
        std::string commandPathPrefix() const override;
};

#endif // Q4XP_FMC_PROFILE_H

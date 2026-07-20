#include "aerobask-phenom-fmc-profile.h"

#include "dataref.h"

// Aerobask Phenom 300: its pedestal keyboard is X-Plane's GCU 478, so the shared profile covers it as is.

AerobaskPhenomFMCProfile::AerobaskPhenomFMCProfile(ProductFMC *product) :
    GCU478FMCProfile(product) {}

bool AerobaskPhenomFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

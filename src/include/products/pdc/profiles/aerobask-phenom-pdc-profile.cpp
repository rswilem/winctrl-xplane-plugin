#include "aerobask-phenom-pdc-profile.h"

#include "dataref.h"

// Aerobask Phenom 300: a PFD and an MFD on X-Plane's G1000. The map range works the MFD map,
// not the PFD inset.

AerobaskPhenomPDCProfile::AerobaskPhenomPDCProfile(ProductPDC *product) :
    G1000PDCProfile(product, true) {}

bool AerobaskPhenomPDCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

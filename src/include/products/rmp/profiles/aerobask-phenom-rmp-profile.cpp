#include "aerobask-phenom-rmp-profile.h"

#include "dataref.h"

// Aerobask Phenom 300: its G1000 radios are X-Plane's own, lit by the panel lighting, so the
// shared profile covers it as is.

AerobaskPhenomRMPProfile::AerobaskPhenomRMPProfile(ProductRMP *product) :
    XPlaneRadiosRMPProfile(product) {}

bool AerobaskPhenomRMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

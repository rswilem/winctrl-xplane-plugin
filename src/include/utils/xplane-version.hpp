#ifndef XPLANE_VERSION_HPP
#define XPLANE_VERSION_HPP

#include "XPLMUtilities.h"

// Helper function to get X-Plane major version at runtime
inline int getXPlaneMajorVersion() {
    int xPlaneVersion = 0;
    XPLMGetVersions(&xPlaneVersion, nullptr, nullptr);
    return xPlaneVersion / 1000;
}

template<typename T>
inline T ifXPlane11(T xp11Value, T xp12Value) {
    return getXPlaneMajorVersion() == 11 ? xp11Value : xp12Value;
}

#endif

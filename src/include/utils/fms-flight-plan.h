#ifndef FMS_FLIGHT_PLAN_H
#define FMS_FLIGHT_PLAN_H

#include <string>
#include <vector>
#include <XPLMNavigation.h>

// A copy of one of X-Plane's built-in flight plans, the ones its G1000, GNS and FMS use
struct FMSWaypoint {
        std::string id;
        XPLMNavType type = xplm_Nav_Unknown;
        int altitude = 0;
        float latitude = 0.0f;
        float longitude = 0.0f;
};

struct FMSFlightPlan {
        std::vector<FMSWaypoint> waypoints;
        // The waypoint being flown to, or -1 when there is none
        int activeIndex = -1;

        static FMSFlightPlan read(XPLMNavFlightPlan plan);
};

#endif

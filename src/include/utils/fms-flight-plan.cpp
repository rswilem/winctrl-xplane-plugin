#include "fms-flight-plan.h"

FMSFlightPlan FMSFlightPlan::read(XPLMNavFlightPlan plan) {
    FMSFlightPlan flightPlan;

    int count = XPLMCountFMSFlightPlanEntries(plan);
    for (int i = 0; i < count; i++) {
        FMSWaypoint waypoint;
        // The SDK asks for at least 256 characters
        char id[256] = {};
        XPLMGetFMSFlightPlanEntryInfo(plan, i, &waypoint.type, id, nullptr, &waypoint.altitude, &waypoint.latitude, &waypoint.longitude);
        waypoint.id = id;
        flightPlan.waypoints.push_back(waypoint);
    }

    if (count > 0) {
        flightPlan.activeIndex = XPLMGetDestinationFMSFlightPlanEntry(plan);
    }

    return flightPlan;
}

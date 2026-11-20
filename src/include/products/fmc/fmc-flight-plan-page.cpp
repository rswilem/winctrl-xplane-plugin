#include "fmc-flight-plan-page.h"

#include "dataref.h"
#include "fms-flight-plan.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <XPLMProcessing.h>
#include <XPLMScenery.h>

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";

    // X-Plane keeps the flight plan in its primary flight plan, and a loaded approach in its
    // approach flight plan. The G1000 and the first GNS use the pilot's.
    constexpr XPLMNavFlightPlan kPrimaryPlan = xplm_Fpl_Pilot_Primary;
    constexpr XPLMNavFlightPlan kApproachPlan = xplm_Fpl_Pilot_Approach;
    // The GPS's desired track for the active leg, and the waypoint it leads to
    constexpr const char *kActiveCourse = "sim/cockpit/radios/gps_course_degtm";
    constexpr const char *kActiveWaypoint = "sim/cockpit2/radios/indicators/gps_nav_id";

    // Button entries that name a scroll action instead of a dataref or command
    constexpr const char *kScrollUp = "flight plan page up";
    constexpr const char *kScrollDown = "flight plan page down";

    // The flight plans are not datarefs, so the page is redrawn on a timer and sent only when it changed
    constexpr float kFlightPlanPollInterval = 0.5f;

    // Colour keys in the page, see colorMap()
    constexpr char kWhite = 'w';
    constexpr char kCyan = 'c';
    constexpr char kMagenta = 'm';
    constexpr char kGrey = 'g';

    // Stands for the degree sign in the page, see mapCharacter()
    constexpr char kDegrees = '`';

    constexpr int kTitleLine = 0;
    constexpr int kHeaderLine = 1;
    constexpr int kFirstRowLine = 2;
    constexpr int kRowsPerPage = ProductFMC::PageLines - kFirstRowLine;

    // A nautical mile is one minute of arc, which is how X-Plane's GPS units measure distance
    constexpr double kEarthRadiusNm = 180.0 * 60.0 / std::numbers::pi;

    double toRadians(double degrees) {
        return degrees * std::numbers::pi / 180.0;
    }

    double distanceNm(double fromLat, double fromLon, double toLat, double toLon) {
        double dLat = toRadians(toLat - fromLat);
        double dLon = toRadians(toLon - fromLon);
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) + std::cos(toRadians(fromLat)) * std::cos(toRadians(toLat)) * std::sin(dLon / 2) * std::sin(dLon / 2);
        return 2 * kEarthRadiusNm * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    }

    // The initial great circle course, in degrees true
    double courseDegrees(double fromLat, double fromLon, double toLat, double toLon) {
        double dLon = toRadians(toLon - fromLon);
        double y = std::sin(dLon) * std::cos(toRadians(toLat));
        double x = std::cos(toRadians(fromLat)) * std::sin(toRadians(toLat)) - std::sin(toRadians(fromLat)) * std::cos(toRadians(toLat)) * std::cos(dLon);
        return std::fmod(std::atan2(y, x) * 180.0 / std::numbers::pi + 360.0, 360.0);
    }

    std::string formatCourse(double degreesMagnetic) {
        int degrees = (static_cast<int>(std::lround(degreesMagnetic)) % 360 + 360) % 360;
        char buf[8];
        snprintf(buf, sizeof(buf), "%03d%c", degrees, kDegrees);
        return buf;
    }

    // A leg's course in degrees magnetic, with the variation where it starts like X-Plane's GPS units.
    // X-Plane gives an easterly variation as negative, so it is added to the true course.
    double legCourse(const FMSWaypoint &from, const FMSWaypoint &to) {
        return courseDegrees(from.latitude, from.longitude, to.latitude, to.longitude) + XPLMGetMagneticVariation(from.latitude, from.longitude);
    }

    // A leg shorter than this has no course, like the one between a procedure's last waypoint
    // and the same waypoint starting the en-route part of the plan
    constexpr double kZeroLengthLegNm = 0.01;

    // Like X-Plane's GPS units: tenths rounded below 100 nm, whole miles cut off above
    std::string formatDistance(double nm) {
        char buf[16];
        if (nm < 100.0) {
            snprintf(buf, sizeof(buf), "%.1f", nm);
        } else {
            snprintf(buf, sizeof(buf), "%.0f", std::floor(nm));
        }
        return buf;
    }

    std::vector<std::vector<char>> blankPage() {
        return std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageBytesPerLine, ' '));
    }
} // namespace

FMCFlightPlanPage::FMCFlightPlanPage(ProductFMC *product) :
    product(product) {
    XPLMRegisterFlightLoopCallback(pollLoop, -1.0f, this);
}

FMCFlightPlanPage::~FMCFlightPlanPage() {
    XPLMUnregisterFlightLoopCallback(pollLoop, this);
}

FMCButtonDef FMCFlightPlanPage::scrollUpButton() {
    return {std::vector<FMCKey>{FMCKey::LSK1L, FMCKey::LSK1R}, kScrollUp};
}

FMCButtonDef FMCFlightPlanPage::scrollDownButton() {
    return {std::vector<FMCKey>{FMCKey::LSK6L, FMCKey::LSK6R}, kScrollDown};
}

bool FMCFlightPlanPage::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || (button->dataref != kScrollUp && button->dataref != kScrollDown)) {
        return false;
    }

    if (phase == xplm_CommandBegin) {
        scroll(button->dataref == kScrollUp ? -1 : 1);
    }
    return true;
}

void FMCFlightPlanPage::scroll(int pages) {
    scrolledPage = std::max(0, shownPage + pages);
    // Show it now rather than at the next poll
    refresh();
}

float FMCFlightPlanPage::pollLoop(float, float, int, void *refcon) {
    static_cast<FMCFlightPlanPage *>(refcon)->refresh();
    return kFlightPlanPollInterval;
}

void FMCFlightPlanPage::refresh() {
    std::vector<std::vector<char>> page = blankPage();
    if (Dataref::getInstance()->getCached<bool>(kAvionicsOn)) {
        Legs legs = listLegs();
        choosePage(legs);
        draw(page, legs);
    }

    if (page == drawnPage) {
        return;
    }

    drawnPage = page;
    product->updatePage(true);
}

FMCFlightPlanPage::Legs FMCFlightPlanPage::listLegs() const {
    auto dataref = Dataref::getInstance();
    FMSFlightPlan primary = FMSFlightPlan::read(kPrimaryPlan);
    FMSFlightPlan approach = FMSFlightPlan::read(kApproachPlan);

    Legs legs;
    double aircraftLat = dataref->get<double>("sim/flightmodel/position/latitude");
    double aircraftLon = dataref->get<double>("sim/flightmodel/position/longitude");
    const FMSWaypoint *previous = nullptr;

    // Both plans keep an active waypoint while an approach is loaded, so the one the GPS is flying to
    // tells them apart. Should both have it, as an IAF on the en-route part can, the approach is not active yet.
    std::string gpsWaypoint = dataref->get<std::string>(kActiveWaypoint);
    auto isFlyingTo = [&gpsWaypoint](const FMSFlightPlan &plan) {
        return plan.activeIndex >= 0 && plan.activeIndex < static_cast<int>(plan.waypoints.size()) && plan.waypoints[plan.activeIndex].id == gpsWaypoint;
    };
    bool flyingApproach = isFlyingTo(approach) && !isFlyingTo(primary);

    auto addLegs = [&](const FMSFlightPlan &plan, int activeIndex) {
        for (int i = 0; i < static_cast<int>(plan.waypoints.size()); i++) {
            const FMSWaypoint &waypoint = plan.waypoints[i];
            bool active = i == activeIndex;

            std::string course;
            std::string distance;
            if (previous) {
                double legLength = distanceNm(previous->latitude, previous->longitude, waypoint.latitude, waypoint.longitude);
                distance = formatDistance(legLength);
                if (legLength >= kZeroLengthLegNm) {
                    course = formatCourse(legCourse(*previous, waypoint));
                }
            }
            if (active) {
                // The active leg counts down from the aircraft. Its course is the GPS's own: after a
                // Direct-To it starts where the aircraft was, which the flight plan does not record.
                distance = formatDistance(distanceNm(aircraftLat, aircraftLon, waypoint.latitude, waypoint.longitude));
                course = formatCourse(dataref->get<float>(kActiveCourse));
            }

            // Airports carry their elevation, other waypoints their altitude constraint or 0
            std::string altitude;
            if (waypoint.type != xplm_Nav_Airport && waypoint.altitude > 0) {
                altitude = std::to_string(waypoint.altitude);
            }

            char text[32];
            snprintf(text, sizeof(text), "%-7.7s %-4s %5s %5s", waypoint.id.c_str(), course.c_str(), distance.c_str(), altitude.c_str());

            if (active) {
                legs.activeRow = static_cast<int>(legs.rows.size());
            }
            legs.rows.push_back({std::string(text).substr(0, ProductFMC::PageCharsPerLine), active ? kMagenta : (i < activeIndex ? kGrey : kWhite)});
            previous = &waypoint;
        }
    };

    // While the approach is flown, the whole en-route plan is behind
    addLegs(primary, flyingApproach ? static_cast<int>(primary.waypoints.size()) : primary.activeIndex);
    if (!approach.waypoints.empty()) {
        legs.rows.push_back({"APPROACH", kCyan, true});
        addLegs(approach, flyingApproach ? approach.activeIndex : -1);
    }

    return legs;
}

int FMCFlightPlanPage::Legs::pageCount() const {
    return (static_cast<int>(rows.size()) + kRowsPerPage - 1) / kRowsPerPage;
}

void FMCFlightPlanPage::choosePage(const Legs &legs) {
    if (legs.activeRow != lastActiveRow) {
        lastActiveRow = legs.activeRow;
        scrolledPage = -1;
    }

    int lastPage = std::max(0, legs.pageCount() - 1);
    shownPage = scrolledPage >= 0 ? std::min(scrolledPage, lastPage) : legs.activeRow / kRowsPerPage;
}

void FMCFlightPlanPage::draw(std::vector<std::vector<char>> &page, const Legs &legs) const {
    product->writeLineToPage(page, kTitleLine, 0, "ACTIVE FLIGHT PLAN", kWhite);
    if (legs.rows.empty()) {
        product->writeLineToPage(page, ProductFMC::PageLines / 2, 5, "NO FLIGHT PLAN", kWhite);
        return;
    }

    std::string pageNumber = std::to_string(shownPage + 1) + "/" + std::to_string(legs.pageCount());
    product->writeLineToPage(page, kTitleLine, ProductFMC::PageCharsPerLine - static_cast<int>(pageNumber.size()), pageNumber, kWhite);

    // Lined up with the row columns: WPT, DTK, then DIS and ALT right-aligned
    product->writeLineToPage(page, kHeaderLine, 0, "WPT     DTK    DIS   ALT", kCyan, true);

    for (int line = 0; line < kRowsPerPage; line++) {
        int row = shownPage * kRowsPerPage + line;
        if (row >= static_cast<int>(legs.rows.size())) {
            break;
        }
        product->writeLineToPage(page, kFirstRowLine + line, 0, legs.rows[row].text, legs.rows[row].color, legs.rows[row].fontSmall);
    }
}

void FMCFlightPlanPage::updatePage(std::vector<std::vector<char>> &page) const {
    page = drawnPage.empty() ? blankPage() : drawnPage;
}

const std::map<char, FMCTextColor> &FMCFlightPlanPage::colorMap() {
    static const std::map<char, FMCTextColor> colors = {
        {kWhite, FMCTextColor::COLOR_WHITE},
        {kCyan, FMCTextColor::COLOR_CYAN},
        {kMagenta, FMCTextColor::COLOR_MAGENTA},
        {kGrey, FMCTextColor::COLOR_GREY},
    };
    return colors;
}

bool FMCFlightPlanPage::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character) {
    if (character != kDegrees) {
        return false;
    }

    buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
    return true;
}

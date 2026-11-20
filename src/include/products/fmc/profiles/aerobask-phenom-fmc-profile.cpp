#include "aerobask-phenom-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <XPLMProcessing.h>
#include <XPLMScenery.h>
#include <XPLMUtilities.h>

// Aerobask Phenom 300: its pedestal keyboard is X-Plane's GCU 478, mapped onto the FMC. The FMS
// knob is on the arrow keys: left/right turn the outer ring, up/down the inner one. Where X-Plane
// exposes the flight plan API (XP12), the screen shows the G1000 flight plan, scrolled with the
// line select keys; elsewhere it stays dark and only the keys are used.

namespace {
    constexpr const char *kAvionicsOn = "sim/cockpit/electrical/avionics_on";
    constexpr const char *kPanelBrightness = "sim/cockpit2/electrical/panel_brightness_ratio";
    constexpr const char *kDisplayBrightness = "sim/cockpit2/electrical/instrument_brightness_ratio_manual";
    constexpr const char *kActiveCourse = "sim/cockpit/radios/gps_course_degtm";
    constexpr const char *kActiveWaypoint = "sim/cockpit2/radios/indicators/gps_nav_id";

    const std::string kGcu = "sim/GPS/gcu478/";

    constexpr int kPilotPrimaryPlan = 0;
    constexpr int kPilotApproachPlan = 2;

    constexpr const char *kScrollUp = "flight plan page up";
    constexpr const char *kScrollDown = "flight plan page down";

    constexpr float kFlightPlanPollInterval = 0.5f;

    constexpr char kWhite = 'w';
    constexpr char kCyan = 'c';
    constexpr char kMagenta = 'm';
    constexpr char kGrey = 'g';
    constexpr char kDegrees = '`';

    constexpr int kTitleLine = 0;
    constexpr int kHeaderLine = 1;
    constexpr int kFirstRowLine = 2;
    constexpr int kRowsPerPage = ProductFMC::PageLines - kFirstRowLine;

    constexpr double kEarthRadiusNm = 180.0 * 60.0 / std::numbers::pi;
    constexpr double kZeroLengthLegNm = 0.01;

    double toRadians(double degrees) {
        return degrees * std::numbers::pi / 180.0;
    }

    double distanceNm(double fromLat, double fromLon, double toLat, double toLon) {
        double dLat = toRadians(toLat - fromLat);
        double dLon = toRadians(toLon - fromLon);
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) + std::cos(toRadians(fromLat)) * std::cos(toRadians(toLat)) * std::sin(dLon / 2) * std::sin(dLon / 2);
        return 2 * kEarthRadiusNm * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    }

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

AerobaskPhenomFMCProfile::AerobaskPhenomFMCProfile(ProductFMC *product) :
    FMCAircraftProfile(product) {
    countEntries = reinterpret_cast<CountEntriesFn>(XPLMFindSymbol("XPLMCountFMSFlightPlanEntries"));
    destinationEntry = reinterpret_cast<DestinationEntryFn>(XPLMFindSymbol("XPLMGetDestinationFMSFlightPlanEntry"));
    entryInfo = reinterpret_cast<EntryInfoFn>(XPLMFindSymbol("XPLMGetFMSFlightPlanEntryInfo"));

    product->setAllLedsEnabled(false);
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, 0);

    // The keys follow the panel lighting like the MCP and PDC, the screen the G1000 displays
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kPanelBrightness, [this](const std::vector<float> &) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>(kAvionicsOn, [this](bool) {
        updateBacklight();
    },
        this);

    if (hasFlightPlan()) {
        Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(kDisplayBrightness, [this](const std::vector<float> &) {
            updateBacklight();
        },
            this);

        XPLMRegisterFlightLoopCallback(pollLoop, -1.0f, this);
    }

    // The monitors only report changes, so a dark panel would otherwise keep the product's default brightness
    updateBacklight();
}

AerobaskPhenomFMCProfile::~AerobaskPhenomFMCProfile() {
    if (hasFlightPlan()) {
        XPLMUnregisterFlightLoopCallback(pollLoop, this);
    }
}

bool AerobaskPhenomFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

const std::vector<std::string> &AerobaskPhenomFMCProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {};
    return datarefs;
}

const std::vector<FMCButtonDef> &AerobaskPhenomFMCProfile::buttonDefs() const {
    // Keys are held for as long as the hardware key, so the cockpit key moves with it
    static const std::vector<FMCButtonDef> buttons = {
        {std::vector<FMCKey>{FMCKey::MCDU_FPLN, FMCKey::PFP_LEGS, FMCKey::PFP_ROUTE}, kGcu + "fpl"},
        {std::vector<FMCKey>{FMCKey::MCDU_AIRPORT, FMCKey::PFP_DEP_ARR}, kGcu + "proc"},
        {std::vector<FMCKey>{FMCKey::MCDU_DIR, FMCKey::PFP_FIX}, kGcu + "direct"},
        {FMCKey::MENU, kGcu + "menu"},
        {std::vector<FMCKey>{FMCKey::MCDU_OVERFLY, FMCKey::PFP_EXEC}, kGcu + "ent"},
        {FMCKey::CLR, kGcu + "clr"},
        {std::vector<FMCKey>{FMCKey::MCDU_EMPTY_BOTTOM_LEFT, FMCKey::PFP_DEL}, kGcu + "bksp"},

        // FMS knob
        {FMCKey::PAGE_PREV, kGcu + "outer_down"},
        {FMCKey::PAGE_NEXT, kGcu + "outer_up"},
        {FMCKey::MCDU_PAGE_DOWN, kGcu + "inner_down"},
        {FMCKey::MCDU_PAGE_UP, kGcu + "inner_up"},
        {FMCKey::MCDU_EMPTY_TOP_RIGHT, kGcu + "cursor"},

        // What the FMS knob tunes
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, kGcu + "nav"},
        {std::vector<FMCKey>{FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_ATC}, kGcu + "com"},

        {FMCKey::KEY0, kGcu + "0"},
        {FMCKey::KEY1, kGcu + "1"},
        {FMCKey::KEY2, kGcu + "2"},
        {FMCKey::KEY3, kGcu + "3"},
        {FMCKey::KEY4, kGcu + "4"},
        {FMCKey::KEY5, kGcu + "5"},
        {FMCKey::KEY6, kGcu + "6"},
        {FMCKey::KEY7, kGcu + "7"},
        {FMCKey::KEY8, kGcu + "8"},
        {FMCKey::KEY9, kGcu + "9"},
        {FMCKey::PERIOD, kGcu + "dot"},
        {FMCKey::PLUSMINUS, kGcu + "minus"},
        {FMCKey::SPACE, kGcu + "spc"},

        {FMCKey::KEYA, kGcu + "A"},
        {FMCKey::KEYB, kGcu + "B"},
        {FMCKey::KEYC, kGcu + "C"},
        {FMCKey::KEYD, kGcu + "D"},
        {FMCKey::KEYE, kGcu + "E"},
        {FMCKey::KEYF, kGcu + "F"},
        {FMCKey::KEYG, kGcu + "G"},
        {FMCKey::KEYH, kGcu + "H"},
        {FMCKey::KEYI, kGcu + "I"},
        {FMCKey::KEYJ, kGcu + "J"},
        {FMCKey::KEYK, kGcu + "K"},
        {FMCKey::KEYL, kGcu + "L"},
        {FMCKey::KEYM, kGcu + "M"},
        {FMCKey::KEYN, kGcu + "N"},
        {FMCKey::KEYO, kGcu + "O"},
        {FMCKey::KEYP, kGcu + "P"},
        {FMCKey::KEYQ, kGcu + "Q"},
        {FMCKey::KEYR, kGcu + "R"},
        {FMCKey::KEYS, kGcu + "S"},
        {FMCKey::KEYT, kGcu + "T"},
        {FMCKey::KEYU, kGcu + "U"},
        {FMCKey::KEYV, kGcu + "V"},
        {FMCKey::KEYW, kGcu + "W"},
        {FMCKey::KEYX, kGcu + "X"},
        {FMCKey::KEYY, kGcu + "Y"},
        {FMCKey::KEYZ, kGcu + "Z"},

        {std::vector<FMCKey>{FMCKey::LSK1L, FMCKey::LSK1R}, kScrollUp},
        {std::vector<FMCKey>{FMCKey::LSK6L, FMCKey::LSK6R}, kScrollDown},
    };
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &AerobaskPhenomFMCProfile::buttonKeyMap() const {
    static const std::unordered_map<FMCKey, const FMCButtonDef *> map = [this] {
        std::unordered_map<FMCKey, const FMCButtonDef *> keys;
        for (const auto &button : buttonDefs()) {
            std::visit([&](auto &&key) {
                using T = std::decay_t<decltype(key)>;
                if constexpr (std::is_same_v<T, FMCKey>) {
                    keys[key] = &button;
                } else {
                    for (FMCKey k : key) {
                        keys[k] = &button;
                    }
                }
            },
                button.key);
        }
        return keys;
    }();
    return map;
}

const std::map<char, FMCTextColor> &AerobaskPhenomFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colors = {
        {kWhite, FMCTextColor::COLOR_WHITE},
        {kCyan, FMCTextColor::COLOR_CYAN},
        {kMagenta, FMCTextColor::COLOR_MAGENTA},
        {kGrey, FMCTextColor::COLOR_GREY},
    };
    return colors;
}

void AerobaskPhenomFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    if (character == kDegrees) {
        buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
        return;
    }

    buffer->push_back(character);
}

void AerobaskPhenomFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = drawnPage.empty() ? blankPage() : drawnPage;
}

void AerobaskPhenomFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    if (button->dataref == kScrollUp || button->dataref == kScrollDown) {
        if (phase == xplm_CommandBegin && hasFlightPlan()) {
            scroll(button->dataref == kScrollUp ? -1 : 1);
        }
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

bool AerobaskPhenomFMCProfile::hasFlightPlan() const {
    return countEntries && destinationEntry && entryInfo;
}

AerobaskPhenomFMCProfile::FlightPlan AerobaskPhenomFMCProfile::readFlightPlan(int plan) const {
    FlightPlan flightPlan;

    int count = countEntries(plan);
    for (int i = 0; i < count; i++) {
        Waypoint waypoint;
        char id[256] = {};
        entryInfo(plan, i, &waypoint.type, id, nullptr, &waypoint.altitude, &waypoint.latitude, &waypoint.longitude);
        waypoint.id = id;
        flightPlan.waypoints.push_back(waypoint);
    }

    if (count > 0) {
        flightPlan.activeIndex = destinationEntry(plan);
    }

    return flightPlan;
}

void AerobaskPhenomFMCProfile::scroll(int pages) {
    scrolledPage = std::max(0, shownPage + pages);
    refresh();
}

float AerobaskPhenomFMCProfile::pollLoop(float, float, int, void *refcon) {
    static_cast<AerobaskPhenomFMCProfile *>(refcon)->refresh();
    return kFlightPlanPollInterval;
}

void AerobaskPhenomFMCProfile::refresh() {
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

AerobaskPhenomFMCProfile::Legs AerobaskPhenomFMCProfile::listLegs() const {
    auto dataref = Dataref::getInstance();
    FlightPlan primary = readFlightPlan(kPilotPrimaryPlan);
    FlightPlan approach = readFlightPlan(kPilotApproachPlan);

    Legs legs;
    double aircraftLat = dataref->get<double>("sim/flightmodel/position/latitude");
    double aircraftLon = dataref->get<double>("sim/flightmodel/position/longitude");
    const Waypoint *previous = nullptr;

    std::string gpsWaypoint = dataref->get<std::string>(kActiveWaypoint);
    auto isFlyingTo = [&gpsWaypoint](const FlightPlan &plan) {
        return plan.activeIndex >= 0 && plan.activeIndex < static_cast<int>(plan.waypoints.size()) && plan.waypoints[plan.activeIndex].id == gpsWaypoint;
    };
    bool flyingApproach = isFlyingTo(approach) && !isFlyingTo(primary);

    auto addLegs = [&](const FlightPlan &plan, int activeIndex) {
        for (int i = 0; i < static_cast<int>(plan.waypoints.size()); i++) {
            const Waypoint &waypoint = plan.waypoints[i];
            bool active = i == activeIndex;

            std::string course;
            std::string distance;
            if (previous) {
                double legLength = distanceNm(previous->latitude, previous->longitude, waypoint.latitude, waypoint.longitude);
                distance = formatDistance(legLength);
                if (legLength >= kZeroLengthLegNm) {
                    double trueCourse = courseDegrees(previous->latitude, previous->longitude, waypoint.latitude, waypoint.longitude);
                    course = formatCourse(trueCourse + XPLMGetMagneticVariation(previous->latitude, previous->longitude));
                }
            }
            if (active) {
                distance = formatDistance(distanceNm(aircraftLat, aircraftLon, waypoint.latitude, waypoint.longitude));
                course = formatCourse(dataref->get<float>(kActiveCourse));
            }

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

    addLegs(primary, flyingApproach ? static_cast<int>(primary.waypoints.size()) : primary.activeIndex);
    if (!approach.waypoints.empty()) {
        legs.rows.push_back({"APPROACH", kCyan, true});
        addLegs(approach, flyingApproach ? approach.activeIndex : -1);
    }

    return legs;
}

int AerobaskPhenomFMCProfile::Legs::pageCount() const {
    return (static_cast<int>(rows.size()) + kRowsPerPage - 1) / kRowsPerPage;
}

void AerobaskPhenomFMCProfile::choosePage(const Legs &legs) {
    if (legs.activeRow != lastActiveRow) {
        lastActiveRow = legs.activeRow;
        scrolledPage = -1;
    }

    int lastPage = std::max(0, legs.pageCount() - 1);
    shownPage = scrolledPage >= 0 ? std::min(scrolledPage, lastPage) : legs.activeRow / kRowsPerPage;
}

void AerobaskPhenomFMCProfile::draw(std::vector<std::vector<char>> &page, const Legs &legs) const {
    product->writeLineToPage(page, kTitleLine, 0, "ACTIVE FLIGHT PLAN", kWhite);
    if (legs.rows.empty()) {
        product->writeLineToPage(page, ProductFMC::PageLines / 2, 5, "NO FLIGHT PLAN", kWhite);
        return;
    }

    std::string pageNumber = std::to_string(shownPage + 1) + "/" + std::to_string(legs.pageCount());
    product->writeLineToPage(page, kTitleLine, ProductFMC::PageCharsPerLine - static_cast<int>(pageNumber.size()), pageNumber, kWhite);
    product->writeLineToPage(page, kHeaderLine, 0, "WPT     DTK    DIS   ALT", kCyan, true);

    for (int line = 0; line < kRowsPerPage; line++) {
        int row = shownPage * kRowsPerPage + line;
        if (row >= static_cast<int>(legs.rows.size())) {
            break;
        }
        product->writeLineToPage(page, kFirstRowLine + line, 0, legs.rows[row].text, legs.rows[row].color, legs.rows[row].fontSmall);
    }
}

void AerobaskPhenomFMCProfile::updateBacklight() {
    auto dataref = Dataref::getInstance();
    std::vector<float> levels = dataref->getCached<std::vector<float>>(kPanelBrightness);
    bool powered = dataref->getCached<bool>(kAvionicsOn);
    float level = powered && !levels.empty() ? std::clamp(levels[0], 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(FMCLed::BACKLIGHT, static_cast<uint8_t>(level * 255));

    if (!hasFlightPlan()) {
        return;
    }

    std::vector<float> displayLevels = dataref->getCached<std::vector<float>>(kDisplayBrightness);
    float displayLevel = powered && !displayLevels.empty() ? std::clamp(displayLevels[0], 0.0f, 1.0f) : 0.0f;
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, static_cast<uint8_t>(displayLevel * 255));
}

#include "xplane-bindings.h"

#include "config.h"
#include "dataref.h"
#include "logger.hpp"
#include "plugins-menu.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <XPLMUtilities.h>

// X-Plane reserves a fixed range of button indices per device slot in
// sim/joystick/joystick_button_assignments and its prf files.
constexpr int kButtonsPerDevice = 160;
constexpr const char *kButtonAssignmentsRef = "sim/joystick/joystick_button_assignments";
// Below this length a serial cannot be trusted to identify a unit.
constexpr size_t kMinSerialLength = 6;

static std::string normalizeSerial(const std::string &serial) {
    std::string normalized;
    for (char c : serial) {
        if (!isspace((unsigned char) c)) {
            normalized += (char) toupper((unsigned char) c);
        }
    }

    if (normalized == "NONE" || normalized.length() < kMinSerialLength) {
        return "";
    }
    return normalized;
}

// A HID layer may report a truncated serial: macOS hands out only the last 20
// characters of a 24-character WINCTRL serial, while the prf holds the full USB
// one. Match on the shorter being a suffix of the longer.
static bool serialsMatch(const std::string &a, const std::string &b) {
    if (a.length() < kMinSerialLength || b.length() < kMinSerialLength) {
        return false;
    }

    const std::string &longer = a.length() >= b.length() ? a : b;
    const std::string &shorter = a.length() >= b.length() ? b : a;
    return longer.compare(longer.length() - shorter.length(), shorter.length(), shorter) == 0;
}

XPlaneBindings *XPlaneBindings::getInstance() {
    static XPlaneBindings instance;
    return &instance;
}

void XPlaneBindings::reload() {
    loadSlotMapping();

    // Re-register on every reload: clearCache() on plane unload drops the
    // cache entry that drives the per-frame poll for this ref. Unbind first
    // so repeated reloads don't stack duplicate callbacks.
    Dataref::getInstance()->unbind(kButtonAssignmentsRef);
    Dataref::getInstance()->monitorExistingDataref<std::vector<int>>(
        kButtonAssignmentsRef, [this](std::vector<int> assignments) {
            rebuildFromAssignments(assignments);
        },
        this);

    rebuildFromAssignments(Dataref::getInstance()->get<std::vector<int>>(kButtonAssignmentsRef));
}

std::unordered_set<uint16_t> *XPlaneBindings::findUnitButtons(uint32_t vidPid, const std::string &serial) {
    auto productIt = boundButtonsPerUnit.find(vidPid);
    if (productIt == boundButtonsPerUnit.end()) {
        return nullptr;
    }

    for (auto &unit : productIt->second) {
        if (serialsMatch(unit.first, serial)) {
            return &unit.second;
        }
    }
    return nullptr;
}

std::unordered_set<uint16_t> &XPlaneBindings::unitButtons(uint32_t vidPid, const std::string &serial) {
    if (std::unordered_set<uint16_t> *existing = findUnitButtons(vidPid, serial)) {
        return *existing;
    }

    auto &units = boundButtonsPerUnit[vidPid];
    units.emplace_back(serial, std::unordered_set<uint16_t>{});
    return units.back().second;
}

bool XPlaneBindings::isButtonBound(uint16_t vendorId, uint16_t productId, const std::string &serialNumber, uint16_t buttonIndex) {
    uint32_t vidPid = ((uint32_t) vendorId << 16) | productId;

    // Every slot the prf knows for this product carries a serial, so a unit
    // without a match simply has no bindings of its own.
    std::string serial = normalizeSerial(serialNumber);
    if (!serial.empty() && perUnitProducts.contains(vidPid)) {
        std::unordered_set<uint16_t> *buttons = findUnitButtons(vidPid, serial);
        return buttons && buttons->contains(buttonIndex);
    }

    auto it = boundButtons.find(vidPid);
    if (it == boundButtons.end()) {
        return false;
    }

    return it->second.contains(buttonIndex);
}

std::vector<uint16_t> XPlaneBindings::boundButtonsForProduct(uint16_t vendorId, uint16_t productId) {
    auto it = boundButtons.find(((uint32_t) vendorId << 16) | productId);
    if (it == boundButtons.end()) {
        return {};
    }

    std::vector<uint16_t> buttons(it->second.begin(), it->second.end());
    std::sort(buttons.begin(), buttons.end());
    return buttons;
}

void XPlaneBindings::loadSlotMapping() {
    slotDevices.clear();

    char systemPath[512];
    XPLMGetSystemPath(systemPath);
    std::string rootDirectory = systemPath;
    if (rootDirectory.ends_with("/")) {
        rootDirectory.pop_back();
    }

    std::string path = rootDirectory + "/Output/preferences/X-Plane Joystick Settings.prf";
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::getInstance()->warn("XPlaneBindings: could not open %s\n", path.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        unsigned slot, vid, pid;

        // _joy_unique_id{slot} VID:{vendor}PID:{product}, both decimal.
        if (sscanf(line.c_str(), "_joy_unique_id%u VID:%uPID:%u", &slot, &vid, &pid) == 3) {
            slotDevices[slot].vidPid = (vid << 16) | pid;
            continue;
        }

        // _joy_location{slot} USB_{vendor}_{product}_{serial}_{location}_...,
        // vendor and product hex, serial "none" when the device has none.
        char serial[64] = {};
        if (sscanf(line.c_str(), "_joy_location%u USB_%x_%x_%63[^_]_", &slot, &vid, &pid, serial) == 4) {
            SlotDevice &device = slotDevices[slot];
            device.vidPid = (vid << 16) | pid;
            device.serial = normalizeSerial(serial);
        }
    }

    Logger::getInstance()->debug("XPlaneBindings: %zu device slots mapped from %s\n", slotDevices.size(), path.c_str());
}

void XPlaneBindings::rebuildFromAssignments(const std::vector<int> &assignments) {
    boundButtons.clear();
    boundButtonsPerUnit.clear();
    perUnitProducts.clear();

    // A product only qualifies for per-unit matching when every slot the prf
    // lists for it has a serial; one unidentified slot and its bindings could
    // belong to any of the units, so that product falls back to merging.
    std::unordered_map<uint32_t, bool> productHasUnidentifiedSlot;
    for (const auto &[slot, device] : slotDevices) {
        productHasUnidentifiedSlot[device.vidPid] |= device.serial.empty();
    }
    for (const auto &[vidPid, unidentified] : productHasUnidentifiedSlot) {
        if (!unidentified) {
            perUnitProducts.insert(vidPid);
        }
    }

    size_t boundCount = 0;
    for (size_t flatIndex = 0; flatIndex < assignments.size(); flatIndex++) {
        if (assignments[flatIndex] == 0) {
            continue;
        }

        auto it = slotDevices.find((unsigned) (flatIndex / kButtonsPerDevice));
        if (it == slotDevices.end()) {
            continue;
        }

        uint16_t button = (uint16_t) (flatIndex % kButtonsPerDevice);
        boundButtons[it->second.vidPid].insert(button);
        if (perUnitProducts.contains(it->second.vidPid)) {
            unitButtons(it->second.vidPid, it->second.serial).insert(button);
        }
        boundCount++;
    }

    size_t identifiedUnits = 0;
    for (const auto &[id, units] : boundButtonsPerUnit) {
        identifiedUnits += units.size();
    }

    Logger::getInstance()->info("XPlaneBindings: %zu X-Plane-bound buttons across %zu devices, %zu identified per unit\n", boundCount, boundButtons.size(), identifiedUnits);

    for (const auto &[id, buttons] : boundButtons) {
        if ((id >> 16) != WINCTRL_VENDOR_ID) {
            continue;
        }
        for (uint16_t button : buttons) {
            Logger::getInstance()->debug("XPlaneBindings: suppressing button %u on device 0x%04X:0x%04X\n", button, id >> 16, id & 0xFFFF);
        }
    }

    for (const auto &[id, units] : boundButtonsPerUnit) {
        for (const auto &[serial, buttons] : units) {
            Logger::getInstance()->debug("XPlaneBindings: device 0x%04X:0x%04X unit %s has %zu bound button(s)\n", id >> 16, id & 0xFFFF, serial.c_str(), buttons.size());
        }
    }

    // The menu shows the per-device count, so it has to follow a mid-session
    // rebind the moment the user leaves X-Plane's joystick settings.
    PluginsMenu::getInstance()->refreshXPlaneBindingItems();
}

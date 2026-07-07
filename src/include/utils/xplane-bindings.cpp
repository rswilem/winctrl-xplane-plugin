#include "xplane-bindings.h"

#include "config.h"
#include "dataref.h"
#include "logger.hpp"

#include <cstdio>
#include <fstream>
#include <XPLMUtilities.h>

// X-Plane reserves a fixed range of button indices per device slot in
// sim/joystick/joystick_button_assignments and its prf files.
constexpr int kButtonsPerDevice = 160;
constexpr const char *kButtonAssignmentsRef = "sim/joystick/joystick_button_assignments";

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

bool XPlaneBindings::isButtonBound(uint16_t vendorId, uint16_t productId, uint16_t buttonIndex) {
    auto it = boundButtons.find(((uint32_t) vendorId << 16) | productId);
    if (it == boundButtons.end()) {
        return false;
    }

    return it->second.contains(buttonIndex);
}

void XPlaneBindings::loadSlotMapping() {
    slotVidPid.clear();

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

    // _joy_unique_id{slot} VID:{vendor}PID:{product}, both decimal.
    std::string line;
    while (std::getline(file, line)) {
        unsigned slot, vid, pid;
        if (sscanf(line.c_str(), "_joy_unique_id%u VID:%uPID:%u", &slot, &vid, &pid) == 3) {
            slotVidPid[slot] = (vid << 16) | pid;
        }
    }

    Logger::getInstance()->debug("XPlaneBindings: %zu device slots mapped from %s\n", slotVidPid.size(), path.c_str());
}

void XPlaneBindings::rebuildFromAssignments(const std::vector<int> &assignments) {
    boundButtons.clear();

    size_t boundCount = 0;
    for (size_t flatIndex = 0; flatIndex < assignments.size(); flatIndex++) {
        if (assignments[flatIndex] == 0) {
            continue;
        }

        auto it = slotVidPid.find((unsigned) (flatIndex / kButtonsPerDevice));
        if (it == slotVidPid.end()) {
            continue;
        }

        boundButtons[it->second].insert((uint16_t) (flatIndex % kButtonsPerDevice));
        boundCount++;
    }

    Logger::getInstance()->info("XPlaneBindings: %zu X-Plane-bound buttons across %zu devices\n", boundCount, boundButtons.size());

    for (const auto &[id, buttons] : boundButtons) {
        if ((id >> 16) != WINCTRL_VENDOR_ID) {
            continue;
        }
        for (uint16_t button : buttons) {
            Logger::getInstance()->debug("XPlaneBindings: suppressing button %u on device 0x%04X:0x%04X\n", button, id >> 16, id & 0xFFFF);
        }
    }
}

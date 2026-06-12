#ifndef XPLANE_BINDINGS_H
#define XPLANE_BINDINGS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Tracks which hardware buttons the user assigned in X-Plane's joystick
// settings so the plugin stays silent on those: X-Plane executes its own
// binding, and the plugin's built-in action must not fire on top of it.
//
// Live state comes from sim/joystick/joystick_button_assignments (int[3200],
// 20 device slots x 160 buttons, 0 = unassigned), monitored through Dataref
// so mid-session rebinds apply immediately; the array already reflects the
// active control profile for the loaded aircraft. The slot-to-device mapping
// comes from the _joy_unique_id entries in X-Plane Joystick Settings.prf,
// which X-Plane rewrites whenever the user leaves the settings screen.
//
// In-slot indices match the plugin's hardwareButtonIndex one-to-one (verified
// on hardware: MCDU KEY2 = plugin 33 = in-slot 33); only the settings UI
// displays 1-based labels on top of this.
class XPlaneBindings {
    private:
        XPlaneBindings() = default;

        // (vendorId << 16) | productId per device slot, from the prf. A
        // device plugged in mid-session is absent until X-Plane rewrites the
        // file on the next settings-screen visit or exit.
        std::unordered_map<unsigned, uint32_t> slotVidPid;

        // Buttons with an X-Plane assignment, keyed by
        // (vendorId << 16) | productId. Identical units share a VID/PID and
        // merge into one set; a binding on either suppresses both.
        std::unordered_map<uint32_t, std::unordered_set<uint16_t>> boundButtons;

        void loadSlotMapping();
        void rebuildFromAssignments(const std::vector<int> &assignments);

    public:
        static XPlaneBindings *getInstance();

        void reload();
        bool isButtonBound(uint16_t vendorId, uint16_t productId, uint16_t buttonIndex);
};

#endif

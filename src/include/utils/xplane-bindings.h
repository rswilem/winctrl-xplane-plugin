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
// comes from the _joy_unique_id and _joy_location entries in X-Plane Joystick
// Settings.prf, which X-Plane rewrites whenever the user leaves the settings
// screen. X-Plane binds per slot, and the USB serial in _joy_location is what
// identifies which physical unit a slot is, so two identical units keep
// separate bindings.
//
// In-slot indices match the plugin's hardwareButtonIndex one-to-one (verified
// on hardware: MCDU KEY2 = plugin 33 = in-slot 33); only the settings UI
// displays 1-based labels on top of this.
class XPlaneBindings {
    private:
        XPlaneBindings() = default;

        struct SlotDevice {
                uint32_t vidPid = 0;
                std::string serial;
        };

        std::unordered_map<unsigned, SlotDevice> slotDevices;

        // Merged across identical units, used when the prf cannot identify
        // units of this product apart.
        std::unordered_map<uint32_t, std::unordered_set<uint16_t>> boundButtons;

        // Per physical unit: vidPid -> list of (serial, buttons). A list
        // because lookups compare with serialsMatch, not equality.
        std::unordered_map<uint32_t, std::vector<std::pair<std::string, std::unordered_set<uint16_t>>>> boundButtonsPerUnit;

        // Products whose every known slot carries a serial, so boundButtonsPerUnit
        // is authoritative for them.
        std::unordered_set<uint32_t> perUnitProducts;

        void loadSlotMapping();
        void rebuildFromAssignments(const std::vector<int> &assignments);
        std::unordered_set<uint16_t> *findUnitButtons(uint32_t vidPid, const std::string &serial);
        std::unordered_set<uint16_t> &unitButtons(uint32_t vidPid, const std::string &serial);

    public:
        static XPlaneBindings *getInstance();

        void reload();
        bool isButtonBound(uint16_t vendorId, uint16_t productId, const std::string &serialNumber, uint16_t buttonIndex);
        // Every button X-Plane has assigned on this product, merged across
        // identical units: the menu reports per product family, not per unit.
        std::vector<uint16_t> boundButtonsForProduct(uint16_t vendorId, uint16_t productId);
};

#endif

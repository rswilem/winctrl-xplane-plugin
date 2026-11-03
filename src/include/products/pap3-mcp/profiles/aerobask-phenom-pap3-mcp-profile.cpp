#include "aerobask-phenom-pap3-mcp-profile.h"

#include "dataref.h"

// Aerobask Phenom 300. Its G1000 has a GFC 700 autopilot panel, which adds C/O and the bank limit
// to the G1000's own controls, and whose YD key is X-Plane's yaw damper toggle. The mode keys work
// through the MFD's bezel commands, and the knobs have custom commands that also turn them in the
// cockpit.

namespace {
    constexpr const char *kLowBankOn = "sim/cockpit2/annunciators/autopilot_bank_limit";
} // namespace

AerobaskPhenomPAP3MCPProfile::AerobaskPhenomPAP3MCPProfile(ProductPAP3MCP *product) :
    G1000PAP3MCPProfile(product, "g1000n3", {
                                                .speedUp = "aerobask/gfc700_spd_inc",
                                                .speedDown = "aerobask/gfc700_spd_dec",
                                                .headingUp = "aerobask/gfc700_hdg_inc",
                                                .headingDown = "aerobask/gfc700_hdg_dec",
                                                .altitudeUp = "aerobask/gfc700_alti_inc",
                                                .altitudeDown = "aerobask/gfc700_alti_dec",
                                                .noseUp = "aerobask/gfc700_vvi_inc",
                                                .noseDown = "aerobask/gfc700_vvi_dec",
                                                .course1Up = "aerobask/gfc700_crs1_inc",
                                                .course1Down = "aerobask/gfc700_crs1_dec",
                                                .course2Up = "aerobask/gfc700_crs2_inc",
                                                .course2Down = "aerobask/gfc700_crs2_dec",
                                            }) {
    // CSC (Current Speed Control) is the Phenom's basic autothrottle
    buttons[1] = {"SPEED", "aerobask/gfc700_csc", PAP3MCPDatarefType::EXECUTE_CMD_PHASED};
    buttons[11] = {"CWS A", "sim/systems/yaw_damper_toggle", PAP3MCPDatarefType::EXECUTE_CMD_PHASED};
    buttons[14] = {"C/O", "sim/autopilot/knots_mach_toggle", PAP3MCPDatarefType::EXECUTE_CMD_PHASED};
}

bool AerobaskPhenomPAP3MCPProfile::IsEligible() {
    return Dataref::getInstance()->exists("aerobask/aircraft_activated") && Dataref::getInstance()->exists("aerobask/test/knob_test");
}

void AerobaskPhenomPAP3MCPProfile::handleBankAngleSwitch(uint8_t switchByte) {
    // The GFC 700 only has a low bank mode: the 10° and 15° positions turn it on, 20° to 30°
    // turn it off. Only act when the switch moves between the two halves, and not on the
    // position reported when the profile loads.
    int lowBank = (switchByte & (0x02 | 0x04)) != 0 ? 1 : 0;
    bool firstReport = hwLowBank < 0;
    if (lowBank == hwLowBank) {
        return;
    }

    hwLowBank = lowBank;
    bool lowBankOn = Dataref::getInstance()->get<int>(kLowBankOn) != 0;
    if (!firstReport && lowBankOn != (lowBank == 1)) {
        Dataref::getInstance()->executeCommand("sim/autopilot/bank_limit_toggle");
    }
}

#ifndef G1000_PAP3MCP_PROFILE_H
#define G1000_PAP3MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

// The commands the MCP knobs send. A knob left empty sends the G1000 bezel's own command, and the
// speed knob, which the G1000 has none of, X-Plane's airspeed reference that FLC flies. Aircraft
// that animate their knobs through custom commands pass those instead.
struct G1000KnobCommands {
        std::string speedUp;
        std::string speedDown;
        std::string headingUp;
        std::string headingDown;
        std::string altitudeUp;
        std::string altitudeDown;
        std::string noseUp;
        std::string noseDown;
        std::string course1Up;
        std::string course1Down;
        std::string course2Up;
        std::string course2Down;
};

// Shared PAP3 MCP support for aircraft with X-Plane's G1000, whose bezel keys and knobs work its
// autopilot. Aircraft profiles derive from it, add their own eligibility check and add the
// controls of any separate autopilot panel to the buttons and encoders.
class G1000PAP3MCPProfile : public PAP3MCPAircraftProfile {
    protected:
        std::unordered_map<uint16_t, PAP3MCPButtonDef> buttons;
        std::vector<PAP3MCPEncoderDef> encoders;

    private:
        // The G1000 display whose bezel the MCP works, as named in X-Plane's G1000 commands
        const std::string bezel;

        // LEDs are derived from several datarefs, so they are recomputed together
        // and only written when they change
        std::unordered_map<PAP3MCPLed, bool> ledStates;

        // "sim/GPS/<bezel>_<key>"
        std::string bezelCommand(const char *key) const;
        void updateBrightness();
        void updateLeds();

    public:
        // The bezel is "g1000n1" for the pilot's PFD, "g1000n2" for the copilot's PFD or "g1000n3" for the MFD
        G1000PAP3MCPProfile(ProductPAP3MCP *product, const std::string &bezel = "g1000n1", const G1000KnobCommands &customKnobs = {});

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
};

#endif

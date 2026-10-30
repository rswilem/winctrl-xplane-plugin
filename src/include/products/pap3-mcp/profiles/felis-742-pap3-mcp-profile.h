#ifndef FELIS_742_PAP3_MCP_PROFILE_H
#define FELIS_742_PAP3_MCP_PROFILE_H

#include "pap3-mcp-aircraft-profile.h"

#include <string>
#include <unordered_map>
#include <vector>

class Felis742PAP3MCPProfile : public PAP3MCPAircraftProfile {
    private:
        // Positions of the cockpit selectors, as stored in their datarefs

        enum class PitchMode {
            Turb = -1,
            Off = 0,
            VS = 1,
            IAS = 2,
            Mach = 3,
        };

        enum class NavMode {
            INS = 0,
            HDG = 1,
            VORLOC = 2,
            ILS = 3,
            LAND = 4,
        };

        enum class AltitudeMode {
            Sel = -1,
            Off = 0,
            Hold = 1,
        };

        enum class EngagePaddle {
            Off = 0,
            Man = 1,
            Cmd = 2,
        };

        // LEDs are derived from several datarefs, so they are recomputed together
        // and only written when they change
        std::unordered_map<PAP3MCPLed, bool> ledStates;

        void updateLeds();
        void selectPosition(const PAP3MCPButtonDef *button);
        void selectPitchMode(PitchMode target);
        void selectNavMode(NavMode target);
        void selectAltitudeMode(AltitudeMode target);
        void moveEngagePaddle(const std::string &paddle, EngagePaddle target);
        void setFlightDirector(int side, bool on);

    public:
        Felis742PAP3MCPProfile(ProductPAP3MCP *product);

        static bool IsEligible();
        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, PAP3MCPButtonDef> &buttonDefs() const override;
        const std::vector<PAP3MCPEncoderDef> &encoderDefs() const override;
        void updateDisplayData(PAP3MCPDisplayData &data) override;
        void buttonPressed(const PAP3MCPButtonDef *button, XPLMCommandPhase phase) override;
        void encoderRotated(const PAP3MCPEncoderDef *encoder, int8_t delta) override;

        void handleSwitchChanged(uint8_t byteOffset, uint8_t bitMask, bool state) override;
};

#endif

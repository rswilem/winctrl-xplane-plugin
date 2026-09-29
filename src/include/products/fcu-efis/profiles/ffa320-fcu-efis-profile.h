#ifndef FFA320_FCU_EFIS_PROFILE_H
#define FFA320_FCU_EFIS_PROFILE_H

#include "fcu-efis-aircraft-profile.h"

#include <set>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>

class FFA320FCUEfisProfile : public FCUEfisAircraftProfile {
    private:
        static constexpr const char *PanelPath = "Aircraft.Cockpit.Panel.";

        std::string fcuPath = "Aircraft.FMGS.FCU1.";
        std::string displayVersionRef;

        bool retained = false;
        bool loggedMissingTree = false;
        bool loggedRawValues = false;
        bool loggedLampSignals = false;
        bool loggedComposed = false;
        std::set<std::string> loggedMissingCommands;
        double conduction = 0.0;
        int displayVersion = 0;
        int wantedAltitudeStep = -1;
        float lastAltitudeStepToggle = 0.0f;
        struct ModeSwitchPulse {
            float pressedAt = 0.0f;
            bool released = false;
        };
        std::unordered_map<std::string, ModeSwitchPulse> modeSwitchPulses;

        FCUDisplayData snapshot;

        void connectToAircraft();
        void pollAircraft();
        void updateLeds();
        void updateBrightness();
        void buildSnapshot();

        double panel(const std::string &leaf) const;
        double fcu(const std::string &leaf) const;

        void stepBaro(bool captainSide, bool increase);
        void setBaroStd(bool captainSide, bool standard);
        void pressClickObject(const std::string &command, bool down);
        void setAltitudeStep(bool thousands);
        void syncAltitudeStep();
        void pressModeSwitch(const std::string &command, bool down);
        void releaseModeSwitches();
        bool resolves(const std::string &path) const;
        bool runCommand(const std::string &command);

    public:
        FFA320FCUEfisProfile(ProductFCUEfis *product);
        ~FFA320FCUEfisProfile();

        static bool IsEligible();

        const std::vector<std::string> &displayDatarefs() const override;
        const std::unordered_map<uint16_t, FCUEfisButtonDef> &buttonDefs() const override;
        void updateDisplayData(FCUDisplayData &data) override;
        void buttonPressed(const FCUEfisButtonDef *button, XPLMCommandPhase phase) override;

        bool hasEfisLeft() const override {
            return true;
        }

        bool hasEfisRight() const override {
            return true;
        }
};

#endif

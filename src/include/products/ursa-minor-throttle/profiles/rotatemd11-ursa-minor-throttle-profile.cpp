#include "rotatemd11-ursa-minor-throttle-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-ursa-minor-throttle.h"

#include <algorithm>
#include <cmath>

RotateMD11UrsaMinorThrottleProfile::RotateMD11UrsaMinorThrottleProfile(ProductUrsaMinorThrottle *product) : UrsaMinorThrottleAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd", [product](bool hasPower) {
        float panelBrt = hasPower ? Dataref::getInstance()->getCached<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio") : 0.0f;
        uint8_t backlight = hasPower ? static_cast<uint8_t>(std::clamp(panelBrt, 0.0f, 1.0f) * 255) : 0;

        product->setLedBrightness(UrsaMinorThrottleLed::BACKLIGHT, backlight);
        product->setLedBrightness(UrsaMinorThrottleLed::OVERALL_LEDS_AND_LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->forceStateSync();
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio", [](float) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/annun_test_signal", [](int) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/fire_eng_1_alert_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/fire_eng_3_alert_lt");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/fire_eng_1_alert_lt", [product](int lit) {
        bool annunTest = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_1_FIRE, (lit || annunTest) ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/fire_eng_3_alert_lt", [product](int lit) {
        bool annunTest = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
        product->setLedBrightness(UrsaMinorThrottleLed::ENG_2_FIRE, (lit || annunTest) ? 1 : 0);
    }, this);
}

RotateMD11UrsaMinorThrottleProfile::~RotateMD11UrsaMinorThrottleProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool RotateMD11UrsaMinorThrottleProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/systems/gcp_alt_presel_ft");
}

const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &RotateMD11UrsaMinorThrottleProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> buttons = {
        {0, {"ENG 1 master ON", ""}},
        {1, {"ENG 1 master OFF", ""}},
        {2, {"ENG 2 master ON", ""}},
        {3, {"ENG 2 master OFF", ""}},
        {4, {"L Fault", ""}},
        {5, {"R Fault", ""}},
        {6, {"ENG mode CRANK", ""}},
        {7, {"ENG mode NORMAL", ""}},
        {8, {"ENG mode START", ""}},
        {9, {"AT disconnect L", "Rotate/aircraft/controls_c/ats_disc_l"}},
        {10, {"AT disconnect R", "Rotate/aircraft/controls_c/ats_disc_r"}},
        {11, {"TOGA L", ""}},
        {12, {"MCT L", ""}},
        {13, {"CLB L", ""}},
        {14, {"IDLE L", ""}},
        {15, {"REV Idle L", ""}},
        {16, {"REV Full L", ""}},
        {17, {"TOGA R", ""}},
        {18, {"MCT R", ""}},
        {19, {"CLB R", ""}},
        {20, {"IDLE R", ""}},
        {21, {"REV Idle R", ""}},
        {22, {"REV Full R", ""}},
        {23, {"Engine mode selector pushed", ""}},
        {24, {"Rudder trim Reset", ""}},
        {25, {"Rudder trim Nose Left", ""}},
        {26, {"Rudder trim Idle", ""}},
        {27, {"Rudder trim Nose Right", ""}},
        {28, {"Park brake OFF", ""}},
        {29, {"Park brake ON", ""}},
        {30, {"FLAP Full", ""}},
        {31, {"FLAP 3", ""}},
        {32, {"FLAP 2", ""}},
        {33, {"FLAP 1", ""}},
        {34, {"FLAP 0", ""}},
        {35, {"Speedbrake full", ""}},
        {36, {"Speedbrake half", ""}},
        {37, {"Speedbrake stowed", ""}},
        {38, {"Speedbrake armed", ""}},
        {39, {"Reversers active L", ""}},
        {40, {"Reversers active R", ""}},
    };
    return buttons;
}

void RotateMD11UrsaMinorThrottleProfile::buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    Dataref::getInstance()->executeCommand(button->dataref.c_str(), phase);
}

void RotateMD11UrsaMinorThrottleProfile::updateDisplays() {
}

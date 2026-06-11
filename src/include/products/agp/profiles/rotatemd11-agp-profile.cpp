#include "rotatemd11-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

RotateMD11AGPProfile::RotateMD11AGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<bool>("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd", [product](bool hasPower) {
        float panelBrt = hasPower ? Dataref::getInstance()->getCached<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio") : 0.0f;
        uint8_t backlight = hasPower ? static_cast<uint8_t>(std::clamp(panelBrt, 0.0f, 1.0f) * 255) : 0;

        product->setLedBrightness(AGPLed::BACKLIGHT, backlight);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("Rotate/aircraft/systems/light_fgs_panel_brt_ratio", [](float) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/elec_dc_batt_bus_pwrd");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/annun_test_signal", [](int) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_down_l_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_down_f_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_down_r_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_disag_l_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_disag_f_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/systems/gear_disag_r_lt");
        Dataref::getInstance()->executeChangedCallbacksForDataref("Rotate/aircraft/controls/auto_brake");
    }, this);

    auto gearGreenHandler = [product](int) {
        bool annunTest = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
        bool downL = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_down_l_lt") || annunTest;
        bool downF = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_down_f_lt") || annunTest;
        bool downR = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_down_r_lt") || annunTest;
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_LEFT, downL ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, downF ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_RIGHT, downR ? 1 : 0);
    };

    auto gearUnlkHandler = [product](int) {
        bool annunTest = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
        bool disL = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_disag_l_lt") || annunTest;
        bool disF = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_disag_f_lt") || annunTest;
        bool disR = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/gear_disag_r_lt") || annunTest;
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_LEFT, disL ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_CENTER, disF ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_RIGHT, disR ? 1 : 0);
        product->setLedBrightness(AGPLed::LDG_GEAR_LEVER_RED, (disL || disF || disR) ? 1 : 0);
    };

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_down_l_lt", gearGreenHandler, this);
    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_down_f_lt", gearGreenHandler, this);
    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_down_r_lt", gearGreenHandler, this);
    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_disag_l_lt", gearUnlkHandler, this);
    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_disag_f_lt", gearUnlkHandler, this);
    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/systems/gear_disag_r_lt", gearUnlkHandler, this);

    Dataref::getInstance()->monitorExistingDataref<int>("Rotate/aircraft/controls/auto_brake", [product](int mode) {
        bool annunTest = Dataref::getInstance()->getCached<int>("Rotate/aircraft/systems/annun_test_signal") == 1;
        product->setLedBrightness(AGPLed::AUTOBRK_LO_ON, (mode == 1 || annunTest) ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MED_ON, (mode == 2 || annunTest) ? 1 : 0);
        product->setLedBrightness(AGPLed::AUTOBRK_MAX_ON, (mode == 3 || annunTest) ? 1 : 0);
    }, this);
}

RotateMD11AGPProfile::~RotateMD11AGPProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool RotateMD11AGPProfile::IsEligible() {
    return Dataref::getInstance()->exists("Rotate/aircraft/systems/gcp_alt_presel_ft");
}

const std::unordered_map<uint16_t, AGPButtonDef> &RotateMD11AGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {0, {"Autobrake OFF", "Rotate/aircraft/controls/auto_brake", AGPDatarefType::SET_VALUE, 0}},
        {1, {"Brake fan off", ""}},
        {2, {"Autobrake Lo", "Rotate/aircraft/controls/auto_brake", AGPDatarefType::SET_VALUE, 1}},
        {3, {"Autobrake Medium", "Rotate/aircraft/controls/auto_brake", AGPDatarefType::SET_VALUE, 2}},
        {4, {"Autobrake Max", "Rotate/aircraft/controls/auto_brake", AGPDatarefType::SET_VALUE, 3}},
        {5, {"Antiskid on", ""}},
        {6, {"Antiskid off", ""}},
        {7, {"RST Turn left", ""}},
        {8, {"RST Press", ""}},
        {9, {"RST Turn right", ""}},
        {10, {"CHR Turn left", ""}},
        {11, {"CHR Press", ""}},
        {12, {"CHR Turn right", ""}},
        {13, {"Date Turn left", ""}},
        {14, {"Date Press", ""}},
        {15, {"Date Turn right", ""}},
        {16, {"UTC switch GPS", ""}},
        {17, {"UTC switch INT", ""}},
        {18, {"UTC switch SET", ""}},
        {19, {"ET switch RUN", ""}},
        {20, {"ET switch STP", ""}},
        {21, {"ET switch RST", ""}},
        {22, {"TERR ON ND", ""}},
        {23, {"GEAR UP", "sim/flight_controls/landing_gear_up"}},
        {24, {"GEAR DOWN", "sim/flight_controls/landing_gear_down"}},
    };
    return buttons;
}

void RotateMD11AGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == AGPDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }
        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void RotateMD11AGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    double zuluTime = Dataref::getInstance()->get<double>("sim/time/zulu_time_sec");
    int hours = static_cast<int>(zuluTime / 3600) % 24;
    int minutes = static_cast<int>(zuluTime / 60) % 60;
    int seconds = static_cast<int>(zuluTime) % 60;

    std::string utc = SegmentDisplay::fixStringLength(std::to_string(hours), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(minutes), 2) + ":" +
                      SegmentDisplay::fixStringLength(std::to_string(seconds), 2);

    product->setLCDText("", utc, "");
}

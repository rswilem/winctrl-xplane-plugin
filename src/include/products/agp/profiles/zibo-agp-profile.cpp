#include "zibo-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

ZiboAGPProfile::ZiboAGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/electric/instrument_brightness", [product](float brightness) {
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        uint8_t backlightBrightness = static_cast<uint8_t>(brightness * 255);
        product->setLedBrightness(AGPLed::BACKLIGHT, hasPower ? backlightBrightness : 0);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("laminar/B738/electric/instrument_brightness");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/left_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearSafe > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/nose_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearSafe > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/right_gear_safe", [product](float gearSafe) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearSafe > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/left_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_LEFT, gearTransit > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/nose_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_CENTER, gearTransit > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<float>("laminar/B738/annunciator/right_gear_transit", [product](float gearTransit) {
        product->setLedBrightness(AGPLed::LDG_GEAR_UNLK_RIGHT, gearTransit > 0.0f ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("laminar/B738/gpws/draw_terrain_flag", [product](bool terrainOn) {
        product->setLedBrightness(AGPLed::TERRAIN_ON, terrainOn ? 1 : 0);
    }, this);
}

ZiboAGPProfile::~ZiboAGPProfile() {
    Dataref::getInstance()->unbindAll(this);
}

bool ZiboAGPProfile::IsEligible() {
    return Dataref::getInstance()->exists("zibomod/Aircraft_Path");
}

const std::unordered_map<uint16_t, AGPButtonDef> &ZiboAGPProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, AGPButtonDef> buttons = {
        {0, {"Brake fan on", ""}},
        {1, {"Brake fan off", ""}},
        {2, {"Autobrake Lo", ""}},
        {3, {"Autobrake Medium", ""}},
        {4, {"Autobrake Max", ""}},
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
        {22, {"TERR ON ND", "zibo_terrain"}},
        {23, {"GEAR UP", "laminar/B738/push_button/gear_up"}},
        {24, {"GEAR DOWN", "laminar/B738/push_button/gear_down"}},
    };

    return buttons;
}

void ZiboAGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    if (button->dataref == "zibo_terrain") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        std::string command = (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) ? "laminar/B738/EFIS_control/capt/push_button/terr_press" : "laminar/B738/EFIS_control/fo/push_button/terr_press";
        datarefManager->executeCommand(command.c_str(), phase);
        return;
    }

    if (button->dataref.empty()) {
        return;
    }

    if (button->datarefType == AGPDatarefType::LANDING_GEAR) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    } else if (button->datarefType == AGPDatarefType::TERRAIN_ON_ND) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        std::string dataref = (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) ? "sim/cockpit2/EFIS/TERRAIN_on_nd1" : "sim/cockpit2/EFIS/TERRAIN_on_nd2";
        datarefManager->set<bool>(dataref.c_str(), !datarefManager->get<bool>(dataref.c_str()));
    } else if (button->datarefType == AGPDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else if (button->datarefType == AGPDatarefType::EXECUTE_CMD_PHASED) {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void ZiboAGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    std::string chrono = "";
    std::string utc = "";
    std::string elapsed = "";

    product->setLCDText(chrono, utc, elapsed);
}

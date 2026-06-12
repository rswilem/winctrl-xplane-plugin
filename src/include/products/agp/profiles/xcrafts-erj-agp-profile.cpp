#include "xcrafts-erj-agp-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-agp.h"
#include "segment-display.h"

#include <algorithm>
#include <cmath>

XCraftsErjAGPProfile::XCraftsErjAGPProfile(ProductAGP *product) : AGPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("sim/cockpit2/electrical/instrument_brightness_ratio_manual", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 12) {
            return;
        }
        uint8_t backlightBrightness = static_cast<uint8_t>(brightness[12] * 255);
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/battery_on");

        product->setLedBrightness(AGPLed::BACKLIGHT, hasPower ? backlightBrightness : 0);
        product->setLedBrightness(AGPLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(AGPLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/battery_on", [](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("sim/cockpit2/electrical/instrument_brightness_ratio_manual");
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("sim/cockpit/switches/gear_handle_status", [product](int gearStatus) {
        product->setLedBrightness(AGPLed::LDG_GEAR_ARROW_GREEN_CENTER, gearStatus == 1 ? 1 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/MFD1/WX_TERR_status", [product](int terrainStatus) {
        if (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) {
            product->setLedBrightness(AGPLed::TERRAIN_ON, terrainStatus == 2 ? 1 : 0);
        }
    }, this);

    Dataref::getInstance()->monitorExistingDataref<int>("XCrafts/ERJ/MFD2/WX_TERR_status", [product](int terrainStatus) {
        if (product->terrainNDPreference == AGPTerrainNDPreference::FIRST_OFFICER) {
            product->setLedBrightness(AGPLed::TERRAIN_ON, terrainStatus == 2 ? 1 : 0);
        }
    }, this);
}

bool XCraftsErjAGPProfile::IsEligible() {
    return Dataref::getInstance()->exists("XCrafts/ERJ/MFD1/WX_TERR_status");
}

const std::unordered_map<uint16_t, AGPButtonDef> &XCraftsErjAGPProfile::buttonDefs() const {
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
        {22, {"TERR ON ND", "custom_terrain"}},
        {23, {"GEAR UP", "sim/flight_controls/landing_gear_toggle", AGPDatarefType::LANDING_GEAR, 0}},
        {24, {"GEAR DOWN", "sim/flight_controls/landing_gear_toggle", AGPDatarefType::LANDING_GEAR, 1}},
    };

    return buttons;
}

void XCraftsErjAGPProfile::buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();

    // Handle terrain button based on MFD preference
    if (button->dataref == "custom_terrain") {
        if (phase != xplm_CommandBegin) {
            return;
        }
        std::string command = (product->terrainNDPreference == AGPTerrainNDPreference::CAPTAIN) ? "XCrafts/ERJ/MFD1/butt_5_cmnd" : "XCrafts/ERJ/MFD2/butt_5_cmnd";
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
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void XCraftsErjAGPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    std::string chrono = "";
    std::string utc = "";
    std::string elapsed = "";

    product->setLCDText(chrono, utc, elapsed);
}

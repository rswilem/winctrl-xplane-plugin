#include "toliss-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

#include <cstdio>

const char *TolissRMPProfile::rmpName() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "RMP1";
        case RMPDeviceVariant::VARIANT_STBY:
            return "RMP3";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "RMP2";
    }
    return "RMP1";
}

const char *TolissRMPProfile::sideName() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "Capt";
        case RMPDeviceVariant::VARIANT_STBY:
            return "RMP3";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "Co";
    }
    return "Capt";
}

const char *TolissRMPProfile::swapCommand() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "AirbusFBW/RMPSwapCapt";
        case RMPDeviceVariant::VARIANT_STBY:
            return "AirbusFBW/RMP3Swap";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "AirbusFBW/RMPSwapCo";
    }
    return "AirbusFBW/RMPSwapCapt";
}

TolissRMPProfile::TolissRMPProfile(ProductRMP *product) : RMPAircraftProfile(product) {
    std::string rmpAvailRef = std::string("AirbusFBW/") + rmpName() + "Available";

    _displayDatarefs = {
        std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString",
        std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString",
        rmpAvailRef,
    };

    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product, rmpAvailRef](float brightness) {
        bool available = Dataref::getInstance()->getCached<int>(rmpAvailRef.c_str()) != 0;
        bool hasPower = Dataref::getInstance()->getCached<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(RMPLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, available ? 255 : 0);

        product->forceStateSync();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<int>(rmpAvailRef.c_str(), [](int available) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        updateDisplays();
    },
        this);

    std::string lightsRef = std::string("AirbusFBW/") + rmpName() + "Lights_Raw";

    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>(lightsRef.c_str(), [product](const std::vector<float> &brightness) {
        if (brightness.size() < 14) {
            return;
        }

        product->setLedBrightness(RMPLed::VHF1, brightness[1] * 255);
        product->setLedBrightness(RMPLed::VHF2, brightness[2] * 255);
        product->setLedBrightness(RMPLed::VHF3, brightness[3] * 255);
        product->setLedBrightness(RMPLed::HF1, brightness[4] * 255);
        product->setLedBrightness(RMPLed::HF2, brightness[5] * 255);
        product->setLedBrightness(RMPLed::AM, brightness[6] * 255);
        product->setLedBrightness(RMPLed::VOR, brightness[7] * 255);
        product->setLedBrightness(RMPLed::ILS, brightness[8] * 255);
        product->setLedBrightness(RMPLed::ADF, brightness[10] * 255);
        product->setLedBrightness(RMPLed::GLS, brightness[11] * 255);
        product->setLedBrightness(RMPLed::SEL, brightness[12] * 255);
        product->setLedBrightness(RMPLed::NAV, brightness[13] * 255);
    },
        this);

    std::string activeRef = std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString";
    std::string stbyRef = std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString";

    Dataref::getInstance()->monitorExistingDataref<std::string>(activeRef.c_str(), [this](std::string s) {
        updateDisplays();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<std::string>(stbyRef.c_str(), [this](std::string s) {
        updateDisplays();
    },
        this);
}

bool TolissRMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::vector<std::string> &TolissRMPProfile::displayDatarefs() const {
    return _displayDatarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &TolissRMPProfile::buttonDefs() const {
    static std::unordered_map<RMPDeviceVariant, std::unordered_map<uint16_t, RMPButtonDef>> cache;

    if (cache.find(product->deviceVariant) == cache.end()) {
        cache[product->deviceVariant] = {
            {0, {"Flip frequencies", swapCommand()}},
            {1, {"VHF 1", std::string("AirbusFBW/VHF1") + sideName()}},
            {2, {"VHF 2", std::string("AirbusFBW/VHF2") + sideName()}},
            {3, {"VHF 3", std::string("AirbusFBW/VHF3") + sideName()}},
            {4, {"LOAD", ""}},
            {5, {"HF1", std::string("AirbusFBW/HF1") + sideName()}},
            {6, {"HF2", std::string("AirbusFBW/HF2") + sideName()}},
            {7, {"AM", std::string("AirbusFBW/AM") + sideName()}},
            {8, {"NAV", std::string("AirbusFBW/") + rmpName() + "/BackupNavPress"}},
            {9, {"VOR", std::string("AirbusFBW/") + rmpName() + "/BackupNavVORPress"}},
            {10, {"ILS", std::string("AirbusFBW/") + rmpName() + "/BackupNavILSPress"}},
            {11, {"GLS", std::string("AirbusFBW/") + rmpName() + "/BackupNavBFOPress"}},
            {12, {"MLS", ""}},
            {13, {"ADF", std::string("AirbusFBW/") + rmpName() + "/BackupNavADFPress"}},
            {14, {"Knob Large L", std::string("AirbusFBW/") + rmpName() + "FreqDownLrg"}},
            {15, {"Knob Large R", std::string("AirbusFBW/") + rmpName() + "FreqUpLrg"}},
            {16, {"Knob Small L", std::string("AirbusFBW/") + rmpName() + "FreqDownSml"}},
            {17, {"Knob Small R", std::string("AirbusFBW/") + rmpName() + "FreqUpSml"}},
            {18, {"Knob depress", ""}},
            {19, {"Switch On", std::string("AirbusFBW/") + rmpName() + "Switch", RMPDatarefType::SET_VALUE, 1}},
            {20, {"Switch Off", std::string("AirbusFBW/") + rmpName() + "Switch", RMPDatarefType::SET_VALUE, 0}},
        };
    }

    return cache[product->deviceVariant];
}

void TolissRMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == RMPDatarefType::SET_VALUE) {
        if (phase != xplm_CommandBegin) {
            return;
        }

        datarefManager->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}

void TolissRMPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string rmpAvailRef = std::string("AirbusFBW/") + rmpName() + "Available";
    bool available = Dataref::getInstance()->getCached<int>(rmpAvailRef.c_str()) != 0;
    if (!available) {
        product->setDisplayText("      ", "      ");
        return;
    }

    std::string activeRef = std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString";
    std::string stbyRef = std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString";
    std::string activeHz = Dataref::getInstance()->getCached<std::string>(activeRef.c_str());
    std::string stbyHz = Dataref::getInstance()->getCached<std::string>(stbyRef.c_str());

    product->setDisplayText(activeHz, stbyHz);
}

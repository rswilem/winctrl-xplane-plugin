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

std::string TolissRMPProfile::formatFrequency(int hz) {
    if (hz <= 0) {
        return "      ";
    }
    int mhz = hz / 1000000;
    int khz = (hz % 1000000) / 1000;
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%3d.%03d", mhz, khz);
    return std::string(buf);
}

TolissRMPProfile::TolissRMPProfile(ProductRMP *product) : RMPAircraftProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("AirbusFBW/PanelBrightnessLevel", [product](float brightness) {
        bool hasEssentialBusPower = Dataref::getInstance()->get<bool>("AirbusFBW/FCUAvail");
        bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
        uint8_t backlightBrightness = hasPower ? brightness * 255 : 0;

        product->setLedBrightness(RMPLed::BACKLIGHT, backlightBrightness);
        product->setLedBrightness(RMPLed::LCD_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
        product->setLedBrightness(RMPLed::OVERALL_LEDS_BRIGHTNESS, hasEssentialBusPower ? 255 : 0);
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("AirbusFBW/FCUAvail", [](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
    });

    Dataref::getInstance()->monitorExistingDataref<bool>("sim/cockpit/electrical/avionics_on", [this](bool poweredOn) {
        Dataref::getInstance()->executeChangedCallbacksForDataref("AirbusFBW/PanelBrightnessLevel");
        updateDisplays();
    });

    std::string activeRef = std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString";
    std::string stbyRef = std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString";

    Dataref::getInstance()->monitorExistingDataref<int>(activeRef.c_str(), [this](int hz) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<int>(stbyRef.c_str(), [this](int hz) {
        updateDisplays();
    });
}

TolissRMPProfile::~TolissRMPProfile() {
    Dataref::getInstance()->unbind("AirbusFBW/PanelBrightnessLevel");
    Dataref::getInstance()->unbind("AirbusFBW/FCUAvail");
    Dataref::getInstance()->unbind("sim/cockpit/electrical/avionics_on");
    std::string activeRef = std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString";
    std::string stbyRef = std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString";
    Dataref::getInstance()->unbind(activeRef.c_str());
    Dataref::getInstance()->unbind(stbyRef.c_str());
}

bool TolissRMPProfile::IsEligible() {
    return Dataref::getInstance()->exists("AirbusFBW/PanelBrightnessLevel");
}

const std::unordered_map<uint16_t, RMPButtonDef> &TolissRMPProfile::buttonDefs() const {
    static std::unordered_map<RMPDeviceVariant, std::unordered_map<uint16_t, RMPButtonDef>> cache;

    if (cache.find(product->deviceVariant) == cache.end()) {
        cache[product->deviceVariant] = {
            {0, {"Flip frequencies", std::string("AirbusFBW/RMPSwap") + sideName()}},
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
            {17, {"Knob Large R", std::string("AirbusFBW/") + rmpName() + "FreqUpSml"}},
            {18, {"Knob depress", ""}},
            {19, {"Switch On",  std::string("AirbusFBW/") + rmpName() + "Switch", RMPDatarefType::SET_VALUE, 1}},
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

    bool hasPower = Dataref::getInstance()->get<bool>("sim/cockpit/electrical/avionics_on");
    if (!hasPower) {
        product->setDisplayText("      ", "      ");
        return;
    }

    std::string activeRef = std::string("AirbusFBW/") + rmpName() + "/ActiveWindowString";
    std::string stbyRef = std::string("AirbusFBW/") + rmpName() + "/StandbyWindowString";
    int activeHz = Dataref::getInstance()->get<int>(activeRef.c_str());
    int stbyHz = Dataref::getInstance()->get<int>(stbyRef.c_str());

    product->setDisplayText(formatFrequency(activeHz), formatFrequency(stbyHz));
}

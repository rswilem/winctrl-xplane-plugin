#include "ff777-rmp-profile.h"

#include "dataref.h"
#include "product-rmp.h"

const char *FF777RMPProfile::rmpName() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "L";
        case RMPDeviceVariant::VARIANT_STBY:
            return "B";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "R";
    }
    return "L";
}

const char *FF777RMPProfile::sideName() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "left";
        case RMPDeviceVariant::VARIANT_STBY:
            return "center";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "right";
    }
    return "left";
}

const char *FF777RMPProfile::swapCommand() const {
    switch (product->deviceVariant) {
        case RMPDeviceVariant::VARIANT_CAPTAIN:
            return "1-sim/command/radLSwapSwitch_button";
        case RMPDeviceVariant::VARIANT_STBY:
            return "1-sim/command/radBSwapSwitch_button";
        case RMPDeviceVariant::VARIANT_FIRSTOFFICER:
            return "1-sim/command/radRSwapSwitch_button";
    }
    return "1-sim/command/radLSwapSwitch_button";
}

FF777RMPProfile::FF777RMPProfile(ProductRMP *product) : RMPAircraftProfile(product) {
    _displayDatarefs = {
        std::string("1-sim/output/radio/") + sideName() + "Active",
        std::string("1-sim/output/radio/") + sideName() + "Stby",
        std::string("1-sim/ckpt/lamps/rad") + rmpName() + "Off",
    };

    _ledDatarefs = {
        {RMPLed::BACKLIGHT, "1-sim/ckpt/lights/aisle"},
        {RMPLed::VHF1, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "VhfL"},
        {RMPLed::VHF2, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "VhfR"},
        {RMPLed::VHF3, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "VhfC"},
        {RMPLed::HF1, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "HfL"},
        {RMPLed::HF2, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "HfR"},
        {RMPLed::AM, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "Am"},
        {RMPLed::SEL, std::string("1-sim/ckpt/lamps/rad") + rmpName() + "OfsdTun"},
    };

    for (const auto &pair : _ledDatarefs) {
        Dataref::getInstance()->monitorExistingDataref<float>(pair.second.c_str(), [product, pair](float brightness) {
            uint8_t ledBrightness = brightness * 255;

            product->setLedBrightness(pair.first, ledBrightness);
        });
    }

    std::string rmpOffRef = std::string("1-sim/ckpt/lamps/rad") + rmpName() + "Off";
    std::string activeRef = std::string("1-sim/output/radio/") + sideName() + "Active";
    std::string stbyRef = std::string("1-sim/output/radio/") + sideName() + "Stby";

    Dataref::getInstance()->monitorExistingDataref<std::string>(activeRef.c_str(), [this](std::string s) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<std::string>(stbyRef.c_str(), [this](std::string s) {
        updateDisplays();
    });

    Dataref::getInstance()->monitorExistingDataref<std::string>(rmpOffRef.c_str(), [this](std::string s) {
        updateDisplays();
    });
}

FF777RMPProfile::~FF777RMPProfile() {
    for (const auto &pair : _ledDatarefs) {
        Dataref::getInstance()->unbind(pair.second.c_str());
    }

    for (const auto &dataref : _displayDatarefs) {
        Dataref::getInstance()->unbind(dataref.c_str());
    }
}

bool FF777RMPProfile::IsEligible() {
    // FF777 datarefs that don't exist on the FF767
    return Dataref::getInstance()->exists("1-sim/ckpt/mcpApLButton/anim") &&
           Dataref::getInstance()->exists("1-sim/output/mcp/ok");
}

const std::vector<std::string> &FF777RMPProfile::displayDatarefs() const {
    return _displayDatarefs;
}

const std::unordered_map<uint16_t, RMPButtonDef> &FF777RMPProfile::buttonDefs() const {
    static std::unordered_map<RMPDeviceVariant, std::unordered_map<uint16_t, RMPButtonDef>> cache;

    if (cache.find(product->deviceVariant) == cache.end()) {
        cache[product->deviceVariant] = {
            {0, {"Flip frequencies", swapCommand()}},
            {1, {"VHF 1", std::string("1-sim/command/rad") + rmpName() + "LvhfButton_button"}},
            {2, {"VHF 2", std::string("1-sim/command/rad") + rmpName() + "RvhfButton_button"}},
            {3, {"VHF 3", std::string("1-sim/command/rad") + rmpName() + "CvhfButton_button"}},
            {4, {"LOAD", ""}},
            {5, {"HF1", std::string("1-sim/command/rad") + rmpName() + "LhfButton_button"}},
            {6, {"HF2", std::string("1-sim/command/rad") + rmpName() + "RhfButton_button"}},
            {7, {"AM", std::string("1-sim/command/rad") + rmpName() + "amButton_button"}},
            {8, {"NAV", ""}},
            {9, {"VOR", ""}},
            {10, {"ILS", ""}},
            {11, {"GLS", ""}},
            {12, {"MLS", ""}},
            {13, {"ADF", ""}},
            {14, {"Knob Large L", std::string("1-sim/command/rad") + rmpName() + "frBigRotary_rotary-"}},
            {15, {"Knob Large R", std::string("1-sim/command/rad") + rmpName() + "frBigRotary_rotary+"}},
            {16, {"Knob Small L", std::string("1-sim/command/rad") + rmpName() + "frSmallRotary_rotary-"}},
            {17, {"Knob Small R", std::string("1-sim/command/rad") + rmpName() + "frSmallRotary_rotary+"}},
            {18, {"Knob depress", ""}},
            {19, {"Switch On", ""}},
            {20, {"Switch Off", ""}},
        };
    }

    return cache[product->deviceVariant];
}

void FF777RMPProfile::buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) {
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

void FF777RMPProfile::updateDisplays() {
    if (!product) {
        return;
    }

    std::string rmpOffRef = std::string("1-sim/ckpt/lamps/rad") + rmpName() + "Off";
    bool isOff = Dataref::getInstance()->getCached<float>(rmpOffRef.c_str()) > 0.01;
    if (isOff) {
        product->setDisplayText("      ", "      ");
        return;
    }

    std::string activeRef = std::string("1-sim/output/radio/") + sideName() + "Active";
    std::string stbyRef = std::string("1-sim/output/radio/") + sideName() + "Stby";
    std::string activeHz = Dataref::getInstance()->getCached<std::string>(activeRef.c_str());
    std::string stbyHz = Dataref::getInstance()->getCached<std::string>(stbyRef.c_str());

    auto formatHz = [](std::string s) -> std::string {
        if (s == "DATA") {
            return " DATA ";
        }
        if (s.find('.') == std::string::npos && s.length() > 3) {
            s.insert(s.length() - 3, ".");
        }
        return s;
    };

    product->setDisplayText(formatHz(activeHz), formatHz(stbyHz));
}

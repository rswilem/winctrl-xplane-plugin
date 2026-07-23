#include "q4xp-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>

Q4XPFMCProfile::Q4XPFMCProfile(ProductFMC *product) : UNS1FMCProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<std::vector<float>>("FJS/Q4XP/Lights/panelText_LIT", [product](const std::vector<float> &brightness) {
        if (brightness.size() <= 2) {
            return;
        }

        uint8_t target = static_cast<uint8_t>(std::clamp(brightness[2], 0.0f, 1.0f) * 255);
        product->setLedBrightness(FMCLed::BACKLIGHT, target);
    },
        this);
}

bool Q4XPFMCProfile::IsEligible() {
    return Dataref::getInstance()->exists("FJS/Q4XP/cdu1/text_line_0");
}

std::string Q4XPFMCProfile::displayPathPrefix() const {
    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "cdu1" : "cdu2";
    return "FJS/Q4XP/" + cdu;
}

std::string Q4XPFMCProfile::commandPathPrefix() const {
    const std::string fms = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "fms1" : "fms2";
    return "FJS/Q4XP/" + fms;
}

#include "fjs732-fmc-profile.h"

#include "dataref.h"
#include "product-fmc.h"

FJS732FMCProfile::FJS732FMCProfile(ProductFMC *product) : UNS1FMCProfile(product) {
    Dataref::getInstance()->monitorExistingDataref<float>("uns1/fms1/screen_brightness", [product](const float &brightness) {
        uint8_t target = static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255);
        product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
    },
        this);
}

bool FJS732FMCProfile::IsEligible() {
    auto datarefManager = Dataref::getInstance();
    return datarefManager->exists("uns1/cdu1/text_line_0") && datarefManager->exists("FJS/732/Maintenance_Toggle");
}

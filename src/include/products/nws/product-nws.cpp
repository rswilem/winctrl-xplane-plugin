#include "product-nws.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/toliss-nws-profile.h"

#include <typeinfo>

ProductNWS::ProductNWS(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    profile = nullptr;
    menuItemId = -1;

    connect();
}

ProductNWS::~ProductNWS() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductNWS::classIdentifier() {
    return "NWS";
}

const char *ProductNWS::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductNWS::setProfileForCurrentAircraft() {
    if (TolissNWSProfile::IsEligible()) {
        profile = new TolissNWSProfile(this);
        profileReady = true;
    } else {
        profile = nullptr;
        profileReady = false;
    }
}

bool ProductNWS::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    lastLedBrightness.clear();

    setLedBrightness(NWSLed::BACKLIGHT, 0);

    setProfileForCurrentAircraft();

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(NWSLed::BACKLIGHT, 255);

                 AppState::getInstance()->executeAfter(2000, this, [this]() {
                     setLedBrightness(NWSLed::BACKLIGHT, 0);
                 });
             }},
        });

    return true;
}

void ProductNWS::blackout() {
    setLedBrightness(NWSLed::BACKLIGHT, 0);
}

void ProductNWS::setLedBrightness(NWSLed led, uint8_t brightness) {
    int ledInt = static_cast<int>(led);
    auto it = lastLedBrightness.find(ledInt);
    if (it != lastLedBrightness.end() && it->second == brightness) {
        return;
    }
    lastLedBrightness[ledInt] = brightness;

    writeData({0x02, ProductNWS::IdentifierByte, 0xB9, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
    writeData({0x02, ProductNWS::IdentifierByte, 0xC9, 0x00, 0x00, 0x03, 0x49, static_cast<uint8_t>(led), brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

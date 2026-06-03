#include "product-orion-throttle.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"

#include <algorithm>
#include <cmath>
#include <limits>

ProductOrionThrottle::ProductOrionThrottle(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    connect();
}

ProductOrionThrottle::~ProductOrionThrottle() {
    blackout();
    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductOrionThrottle::classIdentifier() {
    return "Orion Throttle";
}

const char *ProductOrionThrottle::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductOrionThrottle::setProfileForCurrentAircraft() {
    profile = new OrionThrottleAircraftProfile(this);
}

bool ProductOrionThrottle::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setProfileForCurrentAircraft();

    std::string vibrationSetting = AppState::getInstance()->readPreference("OrionThrottleVibration", "normal");
    loadVibrationSetting(vibrationSetting);

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setVibration(128);
                 AppState::getInstance()->executeAfter(2000, [this]() {
                     setVibration(0);
                 });
             }},
            {.name = "Vibration", .content = std::vector<MenuItem>{

                                     {.name = "Disabled", .checked = vibrationSetting == "disabled", .content = [this](int itemId) {
                                          AppState::getInstance()->writePreference("OrionThrottleVibration", "disabled");
                                          loadVibrationSetting("disabled");
                                          PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                          PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                      }},
                                     {.name = "Normal", .checked = vibrationSetting == "normal", .content = [this](int itemId) {
                                          AppState::getInstance()->writePreference("OrionThrottleVibration", "normal");
                                          loadVibrationSetting("normal");
                                          PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                          PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                      }},
                                     {.name = "Strong", .checked = vibrationSetting == "strong", .content = [this](int itemId) {
                                          AppState::getInstance()->writePreference("OrionThrottleVibration", "strong");
                                          loadVibrationSetting("strong");
                                          PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                          PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                      }},
                                 }},
        });

    return true;
}

void ProductOrionThrottle::blackout() {
    setVibration(0);
}

void ProductOrionThrottle::update() {
    if (!connected) {
        return;
    }

    USBDevice::update();

    if (Dataref::getInstance()->getCached<int>("sim/time/total_flight_time_sec") > 10) {
        float gForce = Dataref::getInstance()->get<float>("sim/flightmodel/forces/g_nrml");
        float delta = fabs(gForce - lastGForce);
        lastGForce = gForce;

        bool onGround = Dataref::getInstance()->getCached<bool>("sim/flightmodel/failures/onground_any");
        uint8_t vibration = (uint8_t) std::min(255.0f, delta * vibrationMultiplier / (onGround ? 1.0f : 2.0f));
        if (vibration < 6) {
            vibration = 0;
        }

        if (lastVibration != vibration) {
            setVibration(vibration);
            lastVibration = vibration;
        }
    } else if (lastVibration > 0) {
        lastVibration = 0;
        setVibration(lastVibration);
    }

    if (profile) {
        profile->update();
    }
}

void ProductOrionThrottle::setVibration(uint8_t vibration) {
    if (vibrationMultiplier <= std::numeric_limits<float>::epsilon()) {
        return;
    }

    writeData({0x02, 0x01, 0xbf, 0x00, 0x00, 0x03, 0x49, 0x00, vibration, 0x00, 0x00, 0x00, 0x00, 0x00});
    writeData({0x02, 0x01, 0xcf, 0x00, 0x00, 0x03, 0x49, 0x00, vibration, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductOrionThrottle::loadVibrationSetting(const std::string &preference) {
    if (preference == "disabled") {
        vibrationMultiplier = 1.0f;
        setVibration(0);
        vibrationMultiplier = 0.0f;
    } else if (preference == "strong") {
        vibrationMultiplier = 1400.0f;
    } else {
        vibrationMultiplier = 800.0f;
    }
}

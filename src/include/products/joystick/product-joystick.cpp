#include "product-joystick.h"

#include "appstate.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "profiles/toliss-joystick-profile.h"
#include "profiles/zibo-joystick-profile.h"

#include <algorithm>
#include <cmath>

ProductJoystick::ProductJoystick(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName, unsigned char identifierByte, unsigned char motorCode) : USBDevice(hidDevice, vendorId, productId, vendorName, productName), identifierByte(identifierByte), motorCode(motorCode) {
    connect();
}

ProductJoystick::~ProductJoystick() {
    AppState::getInstance()->cancelTasksForOwner(this);
    blackout();

    PluginsMenu::getInstance()->removeItem(menuItemId);

    if (profile) {
        delete profile;
        profile = nullptr;
    }
}

const char *ProductJoystick::classIdentifier() {
    switch (productId) {
        case 0xBC27:
        case 0xBC28:
        case 0xBC2A:
        case 0xBC29:
            return "Ursa Minor Joystick";
        case 0xBEA8:
            return "Orion Joystick";
        default:
            return "Joystick";
    }
}

const char *ProductJoystick::activeProfileName() const {
    return profile ? typeid(*profile).name() : "none";
}

void ProductJoystick::setProfileForCurrentAircraft() {
    if (TolissJoystickProfile::IsEligible()) {
        profile = new TolissJoystickProfile(this);
    } else if (ZiboJoystickProfile::IsEligible()) {
        profile = new ZiboJoystickProfile(this);
    } else {
        profile = nullptr;
    }
}

bool ProductJoystick::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    setLedBrightness(0);
    setVibration(0);

    setProfileForCurrentAircraft();

    profileReady = true;

    std::string vibrationSetting = AppState::getInstance()->readPreference("JoystickVibration", "normal");
    loadVibrationSetting(vibrationSetting);

    std::string lightingSetting = AppState::getInstance()->readPreference("JoystickLighting", "enabled");

    if (lightingSetting == "enabled") {
        setLedBrightness(128);
    }

    menuItemId = PluginsMenu::getInstance()->addItem(
        classIdentifier(),
        std::vector<MenuItem>{
            {.name = "Identify", .content = [this](int menuId) {
                 setLedBrightness(255);
                 setVibration(128);
                 AppState::getInstance()->executeAfter(2000, this, [this]() {
                     setLedBrightness(0);
                     setVibration(0);
                 });
             }},
            {.name = "Vibration", .content = std::vector<MenuItem>{

                                      {.name = "Disabled", .checked = vibrationSetting == "disabled", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("JoystickVibration", "disabled");
                                           loadVibrationSetting("disabled");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                      {.name = "Normal", .checked = vibrationSetting == "normal", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("JoystickVibration", "normal");
                                           loadVibrationSetting("normal");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                      {.name = "Strong", .checked = vibrationSetting == "strong", .content = [this](int itemId) {
                                           AppState::getInstance()->writePreference("JoystickVibration", "strong");
                                           loadVibrationSetting("strong");
                                           PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                           PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                       }},
                                  }},

            {.name = "Lighting", .content = std::vector<MenuItem>{

                                     {.name = "Disabled", .checked = lightingSetting == "disabled", .content = [this](int itemId) {
                                          AppState::getInstance()->writePreference("JoystickLighting", "disabled");
                                          setLedBrightness(0);
                                          PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                          PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                      }},
                                     {.name = "Enabled", .checked = lightingSetting == "enabled", .content = [this](int itemId) {
                                          AppState::getInstance()->writePreference("JoystickLighting", "enabled");
                                          setLedBrightness(128);
                                          PluginsMenu::getInstance()->uncheckSubmenuSiblings(itemId);
                                          PluginsMenu::getInstance()->setItemChecked(itemId, true);
                                      }}}},
        });

    return true;
}

void ProductJoystick::blackout() {
    setLedBrightness(0);
    setVibration(0);
}

void ProductJoystick::update() {
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

void ProductJoystick::setVibration(uint8_t vibration) {
    if (vibrationMultiplier <= std::numeric_limits<float>::epsilon()) {
        return;
    }

    writeData({0x02, identifierByte, motorCode, 0x00, 0x00, 0x03, 0x49, 0x00, vibration, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductJoystick::testVibration(uint8_t testIdentifier, uint8_t motorCodeTest) {
    writeData({0x02, testIdentifier, motorCodeTest, 0x00, 0x00, 0x03, 0x49, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductJoystick::setLedBrightness(uint8_t brightness) {
    if (AppState::getInstance()->readPreference("JoystickLighting", "enabled") == "disabled") {
        brightness = 0;
    }

    writeData({0x02, 0x20, 0xBB, 0x00, 0x00, 0x03, 0x49, 0x00, brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void ProductJoystick::loadVibrationSetting(const std::string &preference) {
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

#include "appstate.h"

#include "config.h"
#include "dataref.h"
#include "power-scheme.h"
#include "SimpleIni.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <fstream>
#include <XPLMProcessing.h>

AppState *AppState::instance = nullptr;

AppState::AppState() {
    pluginInitialized = false;
}

AppState::~AppState() {
    instance = nullptr;
}

AppState *AppState::getInstance() {
    if (instance == nullptr) {
        instance = new AppState();
    }

    return instance;
}

bool AppState::initialize() {
    if (pluginInitialized) {
        return false;
    }

    XPLMRegisterFlightLoopCallback(AppState::Update, REFRESH_INTERVAL_SECONDS_FAST, nullptr);

    WindowsPowerScheme::enableHighPerformance();

    pluginInitialized = true;

#ifdef DEBUG
    Dataref::getInstance()->createCommand(
        PRODUCT_NAME "/debug/disconnect_all_devices", "Disconnects all devices", [this](XPLMCommandPhase inPhase) {
            if (inPhase != xplm_CommandBegin) {
                return;
            }

            Logger::getInstance()->info("Disconnecting all devices via debug command...\n");
            USBController::getInstance()->disconnectAllDevices();
        });
#endif

    Logger::getInstance()->info("Plugin initialized.\n");
    return true;
}

void AppState::deinitialize() {
    if (!pluginInitialized) {
        return;
    }

    Logger::getInstance()->info("Plugin deinitializing...\n");

    WindowsPowerScheme::restorePrevious();

    XPLMUnregisterFlightLoopCallback(AppState::Update, nullptr);

    USBController::getInstance()->destroy();

    Dataref::getInstance()->destroyAllBindings();

    pluginInitialized = false;

    taskQueue.clear();

    instance = nullptr;
}

float AppState::Update(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon) {
    auto appstate = AppState::getInstance();

    appstate->update();

    if (!USBController::getInstance()->anyProfileReady()) {
        return REFRESH_INTERVAL_SECONDS_SLOW;
    }

    return REFRESH_INTERVAL_SECONDS_FAST;
}

void AppState::update() {
    auto now = std::chrono::steady_clock::now();

    for (size_t i = 0; i < taskQueue.size(); ++i) {
        if (now >= taskQueue[i].runAt && taskQueue[i].func) {
            taskQueue[i].func();
        }
    }

    taskQueue.erase(std::remove_if(taskQueue.begin(), taskQueue.end(), [&](auto &task) {
        return now >= task.runAt;
    }),
        taskQueue.end());

    if (!pluginInitialized) {
        return;
    }

    Dataref::getInstance()->update();

    for (auto *device : USBController::getInstance()->devices) {
        device->update();
    }
}

void AppState::executeAfter(int milliseconds, std::function<void()> func) {
    taskQueue.push_back({"", std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds), func});
}

void AppState::executeAfterDebounced(std::string taskName, int milliseconds, std::function<void()> func) {
    auto now = std::chrono::steady_clock::now();
    auto it = std::find_if(taskQueue.begin(), taskQueue.end(), [&](const DelayedTask &t) {
        return t.name == taskName;
    });

    if (it != taskQueue.end()) {
        it->runAt = now + std::chrono::milliseconds(milliseconds);
        it->func = func;
    } else {
        taskQueue.push_back({taskName, now + std::chrono::milliseconds(milliseconds), func});
    }
}

std::string AppState::readPreference(const std::string &key, const std::string &defaultValue) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile((getPluginDirectory() + "/preferences.ini").c_str());
    if (rc < 0) {
        return defaultValue;
    }

    const char *value = ini.GetValue("Preferences", key.c_str(), defaultValue.c_str());
    return std::string(value);
}

void AppState::writePreference(const std::string &key, const std::string &value) {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile((getPluginDirectory() + "/preferences.ini").c_str());
    if (rc < 0) {
        // File might not exist yet, continue
    }

    ini.SetValue("Preferences", key.c_str(), value.c_str());

    rc = ini.SaveFile((getPluginDirectory() + "/preferences.ini").c_str());
    if (rc < 0) {
        Logger::getInstance()->info("Failed to save preferences file.\n");
    }
}

std::string AppState::getPluginDirectory() {
    char systemPath[512];
    XPLMGetSystemPath(systemPath);
    std::string rootDirectory = systemPath;
    if (rootDirectory.ends_with("/")) {
        rootDirectory = rootDirectory.substr(0, rootDirectory.length() - 1); // Remove trailing slash
    }

    return rootDirectory + PLUGIN_DIRECTORY;
}

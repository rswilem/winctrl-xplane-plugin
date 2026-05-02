#ifndef XPLM420
#error This is made to be compiled against the XPLM420 SDK for XP12
#endif

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "plugins-menu.h"
#include "usbcontroller.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>
#include <XPLMDisplay.h>
#include <XPLMPlugin.h>
#include <XPLMProcessing.h>

#if IBM
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
#endif

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, long msg, void *params);
void menuAction(void *mRef, void *iRef);

PLUGIN_API int XPluginStart(char *name, char *sig, char *desc) {
    strcpy(name, FRIENDLY_NAME);
    strcpy(sig, BUNDLE_ID);
    strcpy(desc, "WINCTRL X-Plane plugin");
    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
    XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS", 1);
    XPLMEnableFeature("XPLM_WANTS_DATAREF_NOTIFICATIONS", 1);

    // Add "Reload devices" menu item
    PluginsMenu::getInstance()->addPersistentItem("Reload devices", [](int itemIndex) {
        Logger::getInstance()->info("Reloading devices...\n");
        USBController::getInstance()->disconnectAllDevices();
        PluginsMenu::getInstance()->clearAllItems();
        USBController::getInstance()->connectAllDevices();
    });

    // Add "Enable debug logging" menu item
    PluginsMenu::getInstance()->addPersistentItem("Enable debug logging", [](int itemIndex) {
        bool debugLoggingEnabled = !PluginsMenu::getInstance()->isItemChecked(itemIndex);

        PluginsMenu::getInstance()->setItemName(itemIndex, debugLoggingEnabled ? "Debug logging enabled" : "Enable debug logging");
        PluginsMenu::getInstance()->setItemChecked(itemIndex, debugLoggingEnabled);
        Logger::getInstance()->setLogLevel(debugLoggingEnabled ? LogLevel::VERBOSE : LogLevel::INFO);

        if (debugLoggingEnabled) {
            Logger::getInstance()->info("Debug logging was enabled for plugin version %s. Currently connected devices (%lu):\n", VERSION, USBController::getInstance()->devices.size());

            for (auto &device : USBController::getInstance()->devices) {
                Logger::getInstance()->info("- (vendorId: 0x%04X, productId: 0x%04X, handler: %s) %s\n", device->vendorId, device->productId, device->classIdentifier(), device->productName.c_str());
            }

            auto action = std::make_shared<std::function<void()>>();
            *action = [action]() {
                if (Logger::getInstance()->getLogLevel() != LogLevel::VERBOSE) {
                    return;
                }

                auto now = std::chrono::system_clock::now();
                auto nowTimeT = std::chrono::system_clock::to_time_t(now);
                auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

                std::tm localTime;
#if IBM
                localtime_s(&localTime, &nowTimeT);
#else
                localtime_r(&nowTimeT, &localTime);
#endif

                char timeBuffer[9];
                strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &localTime);

                Logger::getInstance()->info("[%s.%03lld] Write queue sizes:\n", timeBuffer, nowMs.count());
                for (auto &device : USBController::getInstance()->devices) {
                    Logger::getInstance()->info("[%s.%03lld] - %s: %zu pending packets\n", timeBuffer, nowMs.count(), device->classIdentifier(), device->getWriteQueueSize());
                }

                AppState::getInstance()->executeAfter(5000, *action);
            };

            (*action)();
        } else {
            Logger::getInstance()->info("Debug logging was disabled.\n");
        }
    });

    Logger::getInstance()->info("Plugin started (version %s)\n", VERSION);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    USBController::getInstance()->disconnectAllDevices();
    PluginsMenu::getInstance()->clearAllItems();
    AppState::getInstance()->deinitialize();
    Logger::getInstance()->info("Plugin stopped\n");
}

PLUGIN_API int XPluginEnable(void) {
    XPluginReceiveMessage(0, XPLM_MSG_PLANE_LOADED, nullptr);

    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    Logger::getInstance()->info("Disabling plugin...\n");
    USBController::getInstance()->disconnectAllDevices();
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID from, long msg, void *params) {
    switch (msg) {
        case XPLM_MSG_PLANE_LOADED: {
            if ((intptr_t) params != 0) {
                // It was not the user's plane. Ignore.
                return;
            }

            AppState::getInstance()->initialize();
            USBController::getInstance()->connectAllDevices();
            break;
        }

        case XPLM_MSG_PLANE_UNLOADED: {
            if ((intptr_t) params != 0) {
                // It was not the user's plane. Ignore.
                return;
            }

            USBController::getInstance()->disconnectAllDevices();
            PluginsMenu::getInstance()->clearAllItems();
            break;
        }

        case XPLM_MSG_AIRPORT_LOADED: {
            break;
        }

        case XPLM_MSG_WILL_WRITE_PREFS:
            // AppState::getInstance()->saveState();
            break;

        default:
            break;
    }
}

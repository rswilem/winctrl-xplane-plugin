#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#if DEBUG
#define debug(format, ...)                                                    \
    {                                                                         \
        char buffer[1024];                                                    \
        snprintf(buffer, sizeof(buffer), "[WINCTRL] " format, ##__VA_ARGS__); \
        if (AppState::getInstance()->debuggingEnabled) {                      \
            XPLMDebugString(buffer);                                          \
        }                                                                     \
        printf("%s", buffer);                                                 \
    }
#define debug_force(format, ...)                                              \
    {                                                                         \
        char buffer[1024];                                                    \
        snprintf(buffer, sizeof(buffer), "[WINCTRL] " format, ##__VA_ARGS__); \
        XPLMDebugString(buffer);                                              \
        printf("%s", buffer);                                                 \
    }
#else
#define debug(format, ...)                                                    \
    {                                                                         \
        char buffer[1024];                                                    \
        snprintf(buffer, sizeof(buffer), "[WINCTRL] " format, ##__VA_ARGS__); \
        if (AppState::getInstance()->debuggingEnabled) {                      \
            XPLMDebugString(buffer);                                          \
        }                                                                     \
    }
#define debug_force(format, ...)                                              \
    {                                                                         \
        char buffer[1024];                                                    \
        snprintf(buffer, sizeof(buffer), "[WINCTRL] " format, ##__VA_ARGS__); \
        XPLMDebugString(buffer);                                              \
    }
#endif

#define PRODUCT_NAME "winctrl"
#define FRIENDLY_NAME "WINCTRL"
#define VERSION "0.0.33"
#define ALL_PLUGINS_DIRECTORY "/Resources/plugins/"
#define PLUGIN_DIRECTORY ALL_PLUGINS_DIRECTORY PRODUCT_NAME
#define BUNDLE_ID "com.ramonster." PRODUCT_NAME

#define REFRESH_INTERVAL_SECONDS_SLOW 5.0
#define REFRESH_INTERVAL_SECONDS_FAST -1

#define WINCTRL_VENDOR_ID 0x4098

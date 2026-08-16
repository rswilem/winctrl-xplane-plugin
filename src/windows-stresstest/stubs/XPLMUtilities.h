#pragma once
// Stub XPLMUtilities.h for standalone Windows stress test build.
// logger.hpp uses XPLMDebugString; stdout via printf() in logger.hpp still works.

#include "diaglog.h"

#ifdef __cplusplus
extern "C" {
#endif

// Routed into the diagnostics log so everything the plugin's Logger emits is
// captured: font.cpp parse messages, and the write-failure / queue-overflow
// warnings from usbdevice_win.cpp that otherwise only reach the console.
static inline void XPLMDebugString(const char *inString) {
    diagLogWrite(inString);
}

static inline void XPLMGetSystemPath(char *outSystemPath) {
    if (outSystemPath) {
        outSystemPath[0] = '\0';
    }
}

#ifdef __cplusplus
}
#endif

typedef int XPLMCommandPhase;
static const XPLMCommandPhase xplm_CommandBegin = 0;
static const XPLMCommandPhase xplm_CommandContinue = 1;
static const XPLMCommandPhase xplm_CommandEnd = 2;

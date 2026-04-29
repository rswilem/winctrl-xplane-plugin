#pragma once
// Stub XPLMUtilities.h for standalone Windows stress test build.
// logger.hpp uses XPLMDebugString; stdout via printf() in logger.hpp still works.

#ifdef __cplusplus
extern "C" {
#endif

static inline void XPLMDebugString(const char *inString) {
    (void) inString;
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

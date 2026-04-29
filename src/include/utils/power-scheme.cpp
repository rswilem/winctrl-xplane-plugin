#include "power-scheme.h"

#include "logger.hpp"

#if IBM
static const GUID kHighPerformanceGuid = {
    0x8c5e7fda, 0xe8bf, 0x4a96,
    {0x9a, 0x85, 0xa6, 0xe2, 0x3a, 0x8c, 0x63, 0x5c}
};


void WindowsPowerScheme::enableHighPerformance() {
    GUID *current = nullptr;
    if (PowerGetActiveScheme(nullptr, &current) == ERROR_SUCCESS) {
        s_previousScheme = *current;
        LocalFree(current);
    }

    if (PowerSetActiveScheme(nullptr, &kHighPerformanceGuid) == ERROR_SUCCESS) {
        s_highPerfEnabled = true;
        Logger::getInstance()->info("High Performance power plan enabled.\n");
    } else {
        Logger::getInstance()->warn("Failed to set High Performance power plan (may need Administrator).\n");
    }
}

void WindowsPowerScheme::restorePrevious() {
    if (!s_highPerfEnabled) {
        return;
    }

    if (PowerSetActiveScheme(nullptr, &s_previousScheme) == ERROR_SUCCESS) {
        Logger::getInstance()->info("High Performance power plan restored.\n");
    } else {
        Logger::getInstance()->warn("Failed to restore previous power plan.\n");
    }

    s_highPerfEnabled = false;
}

bool WindowsPowerScheme::isHighPerfEnabled() {
    return s_highPerfEnabled;
}

#else

void WindowsPowerScheme::enableHighPerformance() {}
void WindowsPowerScheme::restorePrevious() {}
bool WindowsPowerScheme::isHighPerfEnabled() { return false; }

#endif

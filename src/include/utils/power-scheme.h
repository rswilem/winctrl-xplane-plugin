#pragma once

#if IBM
#include <powrprof.h>
#include <windows.h>
#endif

class WindowsPowerScheme {
private:
#if IBM
    static inline GUID s_previousScheme  = {};
    static inline bool s_highPerfEnabled = false;
#endif

public:
    static void enableHighPerformance();
    static void restorePrevious();
    static bool isHighPerfEnabled();
};

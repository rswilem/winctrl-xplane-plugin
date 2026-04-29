#pragma once

#if IBM
// windows.h must come before powrprof.h — powrprof depends on Windows base types.
// clang-format off
#include <windows.h>
#include <powrprof.h>
// clang-format on
#endif

class WindowsPowerScheme {
    private:
#if IBM
        static inline GUID s_previousScheme = {};
        static inline bool s_highPerfEnabled = false;
#endif

    public:
        static void enableHighPerformance();
        static void restorePrevious();
        static bool isHighPerfEnabled();
};

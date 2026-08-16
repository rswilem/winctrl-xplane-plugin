#pragma once

// Single diagnostics log file, shared by the harness and by everything the
// plugin's own Logger emits. The XPLMUtilities.h stub routes XPLMDebugString
// here, so font.cpp messages and the usbdevice_win.cpp write-failure and
// queue-overflow warnings land in the same file as the guided run's output.
// Without that they only ever reached the console and were lost.

#ifdef __cplusplus
extern "C" {
#endif

// Opens the log in append mode. Safe to call more than once; later calls are
// ignored. Written next to the exe.
void diagLogOpen(void);

// Appends a line verbatim (used by the XPLMDebugString stub).
void diagLogWrite(const char *text);

// Appends a printf-formatted line, with a timestamp prefix.
void diagLogPrintf(const char *format, ...);

// Full path of the log file, for telling the user where to find it.
const char *diagLogPath(void);

void diagLogClose(void);

#ifdef __cplusplus
}
#endif

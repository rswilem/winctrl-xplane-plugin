#include "diaglog.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <windows.h>

namespace {
    std::mutex logMutex;
    FILE *logFile = nullptr;
    std::string logPath;

    std::string exeDirectory() {
        char buffer[MAX_PATH] = {0};
        DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) {
            return {};
        }
        std::string path(buffer, length);
        size_t separator = path.find_last_of("\\/");
        return separator == std::string::npos ? std::string() : path.substr(0, separator);
    }

    // Timestamps matter here: the point of the log is to line up device drops
    // with whatever the harness was doing at that moment.
    std::string timestamp() {
        SYSTEMTIME now;
        GetLocalTime(&now);
        char buffer[32] = {0};
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d ", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
        return buffer;
    }
} // namespace

void diagLogOpen(void) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile) {
        return;
    }

    std::string directory = exeDirectory();
    logPath = directory.empty() ? "stresstest-log.txt" : directory + "\\stresstest-log.txt";
    logFile = fopen(logPath.c_str(), "a");
}

void diagLogWrite(const char *text) {
    if (!text || !*text) {
        return;
    }

    std::lock_guard<std::mutex> lock(logMutex);
    if (!logFile) {
        return;
    }

    fputs((timestamp() + text).c_str(), logFile);
    if (text[strlen(text) - 1] != '\n') {
        fputc('\n', logFile);
    }
    fflush(logFile);
}

void diagLogPrintf(const char *format, ...) {
    char buffer[2048] = {0};

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    diagLogWrite(buffer);
}

const char *diagLogPath(void) {
    return logPath.c_str();
}

void diagLogClose(void) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile) {
        fclose(logFile);
        logFile = nullptr;
    }
}

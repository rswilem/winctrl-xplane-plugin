#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string>
#include <XPLMUtilities.h>

#ifndef PRODUCT_NAME
#define PRODUCT_NAME "unknown"
#endif

enum class LogLevel {
    VERBOSE = 0,
    INFO = 1,
    WARN = 2,
    CRITICAL = 3,
};

class Logger {
    public:
        static Logger *getInstance() {
            static Logger instance;
            return &instance;
        }

        void setLogLevel(LogLevel level) {
            std::lock_guard<std::mutex> lock(logMutex);
            currentLogLevel = level;
        }

        LogLevel getLogLevel() const {
            return currentLogLevel;
        }

        void debug(const char *format, ...) {
            va_list args;
            va_start(args, format);
            log(LogLevel::VERBOSE, format, args);
            va_end(args);
        }

        void info(const char *format, ...) {
            va_list args;
            va_start(args, format);
            log(LogLevel::INFO, format, args);
            va_end(args);
        }

        void warn(const char *format, ...) {
            va_list args;
            va_start(args, format);
            log(LogLevel::WARN, format, args);
            va_end(args);
        }

        void error(const char *format, ...) {
            va_list args;
            va_start(args, format);
            log(LogLevel::CRITICAL, format, args);
            va_end(args);
        }

        void logForce(const char *format, ...) {
            va_list args;
            va_start(args, format);
            logInternal(LogLevel::CRITICAL, format, args, true);
            va_end(args);
        }

    private:
        Logger() : currentLogLevel(LogLevel::VERBOSE) {}

        ~Logger() = default;
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        void log(LogLevel level, const char *format, va_list args) {
            logInternal(level, format, args, false);
        }

        void logInternal(LogLevel level, const char *format, va_list args, bool force) {
            if (!force && level < currentLogLevel) {
                return;
            }

            std::lock_guard<std::mutex> lock(logMutex);

            const char *levelStr = "";
            switch (level) {
                case LogLevel::VERBOSE:
                    levelStr = "Debug";
                    break;
                case LogLevel::INFO:
                    levelStr = "Info";
                    break;
                case LogLevel::WARN:
                    levelStr = "Warning";
                    break;
                case LogLevel::CRITICAL:
                    levelStr = "Error";
                    break;
            }

            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, args);

            char finalBuffer[1360];
            snprintf(finalBuffer, sizeof(finalBuffer), "[%s] %s: %s", PRODUCT_NAME, levelStr, buffer);

            XPLMDebugString(finalBuffer);
            printf("%s", finalBuffer);
            fflush(stdout);
        }

        std::mutex logMutex;
        LogLevel currentLogLevel;
};

#endif

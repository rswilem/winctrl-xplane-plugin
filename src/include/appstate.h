#ifndef APPSTATE_H
#define APPSTATE_H

#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

struct DelayedTask {
        std::string name;
        void *owner;
        std::chrono::steady_clock::time_point runAt;
        std::function<void()> func;
};

class AppState {
    private:
        AppState();
        ~AppState();

        static AppState *instance;
        std::vector<DelayedTask> taskQueue;
        std::mutex taskQueueMutex;
        void update();

    public:
        static float Update(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void *inRefcon);

        bool pluginInitialized;

        static AppState *getInstance();
        bool initialize();
        void deinitialize();
        std::string getPluginDirectory();

        void executeAfter(int milliseconds, void *owner, std::function<void()> func);
        void executeAfterDebounced(std::string taskName, int milliseconds, void *owner, std::function<void()> func);
        void cancelTasksForOwner(void *owner);

        std::string readPreference(const std::string &key, const std::string &defaultValue);
        void writePreference(const std::string &key, const std::string &value);
};

#endif

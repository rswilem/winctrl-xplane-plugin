#ifndef DATAREF_H
#define DATAREF_H

#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>
#include <XPLMDataAccess.h>
#include <XPLMUtilities.h>

using DataRefValueType = std::
    variant<float, double, int, bool, std::string, std::vector<int>, std::vector<float>, std::vector<unsigned char>>;
template<typename T>
using DatarefShouldChangeCallback = std::function<bool(T)>;
template<typename T>
using DatarefMonitorChangedCallback = std::function<void(T)>;

struct BoundRef {
        XPLMDataRef handle;
        void *valuePointer;
        std::vector<DatarefShouldChangeCallback<DataRefValueType>> changeCallbacks;
};

typedef std::function<void(XPLMCommandPhase inPhase)> CommandExecutedCallback;

struct BoundCommand {
        XPLMCommandRef handle;
        CommandExecutedCallback callback;
};

struct CachedValue {
        DataRefValueType value;
        int lastUpdateCycleNumber;
};

class Dataref {
    private:
        Dataref();
        ~Dataref();
        static Dataref *instance;
        std::unordered_map<std::string, BoundRef> boundRefs;
        std::unordered_map<std::string, BoundCommand> boundCommands;
        std::unordered_map<std::string, XPLMDataRef> refs;
        std::unordered_map<std::string, CachedValue> cachedValues;
        XPLMDataRef findRef(const char *ref);
        std::thread::id mainThreadId;
        std::mutex taskQueueMutex;
        std::vector<std::function<void()>> taskQueue;
        void drainMainThreadQueue();

    public:
        static Dataref *getInstance();

        template<typename T>
        void monitorExistingDataref(const char *ref, DatarefMonitorChangedCallback<T> callback);
        template<typename T>
        void createDataref(
            const char *ref, T *value, bool writable = false, DatarefShouldChangeCallback<T> changeCallback = nullptr);
        void bindExistingCommand(const char *command, CommandExecutedCallback callback);
        void createCommand(const char *command, const char *description, CommandExecutedCallback callback);
        void unbind(const char *ref);
        void destroyAllBindings();
        int _commandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon);

        void update();
        bool exists(const char *ref);
        void executeChangedCallbacksForDataref(const char *ref);
        int getCachedLastUpdate(const char *ref);
        template<typename T>
        T getCached(const char *ref);
        template<typename T>
        T get(const char *ref);
        template<typename T>
        void set(const char *ref, T value, bool setCacheOnly = false);

        void executeCommand(const char *command, XPLMCommandPhase phase = -1);

        void clearCache();
};

#endif

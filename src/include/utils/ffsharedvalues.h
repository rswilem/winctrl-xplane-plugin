#ifndef FFSHAREDVALUES_H
#define FFSHAREDVALUES_H

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <XPLMPlugin.h>

// FlightFactor's SharedValuesInterface, the value tree behind their aircraft.
// The A320 ultimate publishes only its cockpit click objects as X-Plane
// datarefs and keeps everything else here, the MCDU display included, so this
// is the only way in. Struct layout, message id and type constants are copied
// from the aircraft's own data/SDK/SharedValue.h; the order of the function
// pointers is part of the ABI and must not be rearranged.

#if IBM
#define FF_SHARED_CALL __stdcall
#else
#define FF_SHARED_CALL
#endif

constexpr int FFSharedMsgGetInterface = 1001;

enum FFSharedValueType : unsigned int {
    FFValueTypeDeleted = 0,
    FFValueTypeObject = 1,
    FFValueTypeSint8 = 2,
    FFValueTypeUint8 = 3,
    FFValueTypeSint16 = 4,
    FFValueTypeUint16 = 5,
    FFValueTypeSint32 = 6,
    FFValueTypeUint32 = 7,
    FFValueTypeFloat32 = 8,
    FFValueTypeFloat64 = 9,
    FFValueTypeString = 10,
    FFValueTypeTime = 11,
};

typedef unsigned int(FF_SHARED_CALL *FFSharedDataVersionProc)();
typedef void(FF_SHARED_CALL *FFSharedDataUpdateProc)(double step, void *tag);
typedef void(FF_SHARED_CALL *FFSharedDataAddUpdateProc)(FFSharedDataUpdateProc proc, void *tag);
typedef void(FF_SHARED_CALL *FFSharedDataDelUpdateProc)(FFSharedDataUpdateProc proc, void *tag);
typedef unsigned int(FF_SHARED_CALL *FFSharedValuesCountProc)();
typedef int(FF_SHARED_CALL *FFSharedValueIdByIndexProc)(unsigned int index);
typedef int(FF_SHARED_CALL *FFSharedValueIdByNameProc)(const char *name);
typedef const char *(FF_SHARED_CALL *FFSharedValueNameProc)(int id);
typedef const char *(FF_SHARED_CALL *FFSharedValueDescProc)(int id);
typedef unsigned int(FF_SHARED_CALL *FFSharedValueTypeProc)(int id);
typedef unsigned int(FF_SHARED_CALL *FFSharedValueFlagsProc)(int id);
typedef unsigned int(FF_SHARED_CALL *FFSharedValueUnitsProc)(int id);
typedef int(FF_SHARED_CALL *FFSharedValueParentProc)(int id);
typedef void(FF_SHARED_CALL *FFSharedValueSetProc)(int id, const void *src);
typedef void(FF_SHARED_CALL *FFSharedValueGetProc)(int id, void *dst);
typedef unsigned int(FF_SHARED_CALL *FFSharedValueGetSizeProc)(int id);
typedef void(FF_SHARED_CALL *FFSharedValueReaderProc)(void *dst, unsigned int size, void *tag);
typedef void(FF_SHARED_CALL *FFSharedValueWriterProc)(const void *src, unsigned int size, void *tag);
typedef bool(FF_SHARED_CALL *FFSharedValueObjectLoadStateProc)(int id, FFSharedValueReaderProc src, void *tag);
typedef void(FF_SHARED_CALL *FFSharedValueObjectSaveStateProc)(int id, FFSharedValueWriterProc dst, void *tag);
typedef int(FF_SHARED_CALL *FFSharedValueObjectNewValueProc)(
    int id, const char *name, const char *desc, void *ptr, unsigned int type, unsigned int flags, unsigned int units);

struct FFSharedValuesInterface {
        FFSharedDataVersionProc DataVersion = nullptr;
        FFSharedDataAddUpdateProc DataAddUpdate = nullptr;
        FFSharedDataDelUpdateProc DataDelUpdate = nullptr;
        FFSharedValuesCountProc ValuesCount = nullptr;
        FFSharedValueIdByIndexProc ValueIdByIndex = nullptr;
        FFSharedValueIdByNameProc ValueIdByName = nullptr;
        FFSharedValueNameProc ValueName = nullptr;
        FFSharedValueDescProc ValueDesc = nullptr;
        FFSharedValueTypeProc ValueType = nullptr;
        FFSharedValueFlagsProc ValueFlags = nullptr;
        FFSharedValueUnitsProc ValueUnits = nullptr;
        FFSharedValueParentProc ValueParent = nullptr;
        FFSharedValueSetProc ValueSet = nullptr;
        FFSharedValueGetProc ValueGet = nullptr;
        FFSharedValueGetSizeProc ValueGetSize = nullptr;
        FFSharedValueObjectLoadStateProc ValueObjectLoadState = nullptr;
        FFSharedValueObjectSaveStateProc ValueObjectSaveState = nullptr;
        FFSharedValueObjectNewValueProc ValueObjectNewValue = nullptr;
};

// Paths are dotted, rooted the way the aircraft's own web MCDU addresses them:
// "Aircraft.FMGS.MCDU1.DisplayLines1". Lookups go through ValueIdByName first
// and fall back to an enumerated index of the whole tree, so a different root
// naming in the aircraft still resolves.
//
// Main thread only: every call reaches into the other plugin's value tree.
class FFSharedValues {
    public:
        static constexpr const char *A320UltimateSignature = "FlightFactor.A320.ultimate";

        static FFSharedValues *getInstance();

        // Reference counted so two MCDU devices share one connection. The last
        // release() drops the interface, which is what makes an aircraft change
        // start from a clean tree.
        bool retain(const char *pluginSignature);
        void release();
        bool isConnected() const {
            return iface.ValuesCount != nullptr;
        }

        using UpdateCallback = std::function<void(double step)>;

        // Registered with the aircraft's own per-frame value update, so reads
        // inside the callback see a coherent frame.
        void addUpdateCallback(void *owner, UpdateCallback callback);
        void removeUpdateCallback(void *owner);

        int idForPath(const std::string &path);
        std::string getString(const std::string &path);
        double getNumber(const std::string &path);
        void setNumber(const std::string &path, double value);

    private:
        static FFSharedValues *instance;

        FFSharedValuesInterface iface = {};
        int retainCount = 0;
        unsigned int indexedDataVersion = 0;
        unsigned int indexedValuesCount = 0;
        bool indexBuilt = false;
        bool updateRegistered = false;
        std::unordered_map<std::string, int> resolvedIds;
        std::unordered_map<std::string, int> indexedPaths;
        std::vector<std::pair<void *, UpdateCallback>> updateCallbacks;

        void disconnect();
        void buildIndex();
        void dropStaleIndex();
        std::string pathForId(int id, std::unordered_map<int, std::string> &memo, int depth = 0);
        static void FF_SHARED_CALL updateTrampoline(double step, void *tag);
};

#endif

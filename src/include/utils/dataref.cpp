#include "dataref.h"

#include "appstate.h"
#include "config.h"

#include <cmath>
#include <cstring>
#include <XPLMDisplay.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>

using namespace std;

Dataref *Dataref::instance = nullptr;

int handleCommandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon) {
    return Dataref::getInstance()->_commandCallback(inCommand, inPhase, inRefcon);
}

Dataref::Dataref() {
    cachedValues = {};
    refs = {};
}

Dataref::~Dataref() {
    instance = nullptr;
}

Dataref *Dataref::getInstance() {
    if (instance == nullptr) {
        instance = new Dataref();
    }

    return instance;
}

template void Dataref::createDataref<int>(const char *ref, int *value, bool writable = false, DatarefShouldChangeCallback<int> changeCallback = nullptr);
template void Dataref::createDataref<bool>(const char *ref, bool *value, bool writable = false, DatarefShouldChangeCallback<bool> changeCallback = nullptr);
template void Dataref::createDataref<float>(const char *ref, float *value, bool writable = false, DatarefShouldChangeCallback<float> changeCallback = nullptr);
template void Dataref::createDataref<double>(const char *ref, double *value, bool writable = false, DatarefShouldChangeCallback<double> changeCallback = nullptr);
template void Dataref::createDataref<std::string>(const char *ref, std::string *value, bool writable = false, DatarefShouldChangeCallback<std::string> changeCallback = nullptr);

template<typename T>
void Dataref::createDataref(const char *ref, T *value, bool writable, DatarefShouldChangeCallback<T> changeCallback) {
    unbind(ref);

    XPLMDataRef handle = nullptr;
    boundRefs[ref] = {
        handle,
        value,
        {[changeCallback](DataRefValueType newValue) -> bool {
            if (!changeCallback) {
                return true;
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (std::holds_alternative<std::string>(newValue)) {
                    return changeCallback(std::get<std::string>(newValue));
                }
            } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>) {
                if (std::holds_alternative<int>(newValue)) {
                    return changeCallback(std::get<int>(newValue));
                }
            } else if constexpr (std::is_same_v<T, float>) {
                if (std::holds_alternative<float>(newValue)) {
                    return changeCallback(std::get<float>(newValue));
                }
            } else if constexpr (std::is_same_v<T, double>) {
                if (std::holds_alternative<double>(newValue)) {
                    return changeCallback(std::get<double>(newValue));
                }
            }
            return false;
        }},
    };

    if constexpr ((std::is_same_v<T, int>) || (std::is_same_v<T, bool>) ) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Int,
            writable ? 1 : 0,
            [](void *inRefcon) -> int {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, int inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0](inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Float
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, float>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Float,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            [](void *inRefcon) -> T {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, T inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0](inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, double>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Double,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            nullptr,
            nullptr, // Float
            [](void *inRefcon) -> T {
                return *static_cast<T *>(inRefcon);
            },
            [](void *inRefcon, T inValue) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    if (info->changeCallbacks[0](inValue)) {
                        *valuePtr = inValue;
                    }
                } else {
                    *valuePtr = inValue;
                }
            },
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            nullptr,
            nullptr,          // Binary
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    } else if constexpr (std::is_same_v<T, std::string>) {
        handle = XPLMRegisterDataAccessor(
            ref,
            xplmType_Data,
            writable ? 1 : 0,
            nullptr,
            nullptr, // Int
            nullptr,
            nullptr, // Float
            nullptr,
            nullptr, // Double
            nullptr,
            nullptr, // Int array
            nullptr,
            nullptr, // Float array
            [](void *inRefcon, void *outValue, int inOffset, int inMaxLength) -> int {
                T value = *static_cast<T *>(inRefcon);
                strncpy(static_cast<char *>(outValue), value.c_str(), inMaxLength);
                return static_cast<int>(value.length());
            },
            [](void *inRefcon, void *inValue, int inOffset, int inMaxLength) {
                BoundRef *info = static_cast<BoundRef *>(inRefcon);
                T *valuePtr = static_cast<T *>(info->valuePointer);

                if (info->changeCallbacks.size()) {
                    std::string newValue = std::string(static_cast<const char *>(inValue));
                    if (info->changeCallbacks[0](newValue)) {
                        *valuePtr = (const char *) inValue;
                    }
                } else {
                    *valuePtr = (const char *) inValue;
                }
            },
            value,            // Read refcon
            &boundRefs[ref]); // Write refcon
    }

    boundRefs[ref].handle = handle;
}

template void Dataref::monitorExistingDataref<int>(const char *ref, DatarefMonitorChangedCallback<int> changeCallback);
template void Dataref::monitorExistingDataref<bool>(const char *ref, DatarefMonitorChangedCallback<bool> changeCallback);
template void Dataref::monitorExistingDataref<float>(const char *ref, DatarefMonitorChangedCallback<float> changeCallback);
template void Dataref::monitorExistingDataref<double>(const char *ref, DatarefMonitorChangedCallback<double> changeCallback);
template void Dataref::monitorExistingDataref<std::string>(const char *ref, DatarefMonitorChangedCallback<std::string> changeCallback);
template void Dataref::monitorExistingDataref<std::vector<float>>(const char *ref, DatarefMonitorChangedCallback<std::vector<float>> changeCallback);
template void Dataref::monitorExistingDataref<std::vector<int>>(const char *ref, DatarefMonitorChangedCallback<std::vector<int>> changeCallback);

template<typename T>
void Dataref::monitorExistingDataref(const char *ref, DatarefMonitorChangedCallback<T> changeCallback) {
    if constexpr (std::is_same_v<T, std::string>) {
        set<T>(ref, "", true);
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        set<T>(ref, {}, true);
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        set<T>(ref, {}, true);
    } else {
        set<T>(ref, 0, true);
    }

    auto callback = [changeCallback](DataRefValueType newValue) -> bool {
        if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<int>(newValue)) {
                changeCallback(std::get<int>(newValue));
            } else if (std::holds_alternative<bool>(newValue)) {
                changeCallback(std::get<bool>(newValue));
            } else if (std::holds_alternative<float>(newValue)) {
                changeCallback(std::get<float>(newValue));
            } else if (std::holds_alternative<double>(newValue)) {
                changeCallback(std::get<double>(newValue));
            }
        } else {
            if (std::holds_alternative<T>(newValue)) {
                changeCallback(std::get<T>(newValue));
            }
        }

        return false;
    };

    if (boundRefs.find(ref) != boundRefs.end()) {
        boundRefs[ref].changeCallbacks.push_back(callback);
    } else {
        boundRefs[ref] = {0, nullptr, {callback}};
    }
}

void Dataref::destroyAllBindings() {
    for (auto &[key, ref] : boundRefs) {
        XPLMUnregisterDataAccessor(ref.handle);
    }
    boundRefs.clear();

    for (auto &[key, ref] : boundCommands) {
        XPLMUnregisterCommandHandler(ref.handle, handleCommandCallback, 1, nullptr);
    }
    boundCommands.clear();
}

void Dataref::unbind(const char *ref) {
    auto it = boundRefs.find(ref);
    if (it != boundRefs.end()) {
        if (it->second.handle) {
            XPLMUnregisterDataAccessor(it->second.handle);
        }
        boundRefs.erase(it);
    }

    auto it2 = boundCommands.find(ref);
    if (it2 != boundCommands.end()) {
        XPLMUnregisterCommandHandler(it2->second.handle, handleCommandCallback, 1, nullptr);
        boundCommands.erase(it2);
    }

    //    refs.erase(ref);
    //    cachedValues.erase(ref);
}

void Dataref::clearCache() {
    cachedValues.clear();
}

void Dataref::update() {
    std::vector<std::pair<std::string, CachedValue>> updates;

    for (auto &[key, data] : cachedValues) {
        std::visit(
            [&](auto &&value) {
                using T = std::decay_t<decltype(value)>;
                T newValue = get<T>(key.c_str());
                bool didChange = false;
                if constexpr (std::is_floating_point_v<T>) {
                    didChange = std::fabs(value - newValue) > std::numeric_limits<T>::epsilon();
                } else {
                    didChange = value != newValue;
                }

                if (didChange) {
                    updates.emplace_back(key,
                        CachedValue{
                            .value = newValue,
                            .lastUpdateCycleNumber = XPLMGetCycleNumber(),
                        });
                }
            },
            data.value);
    }

    for (auto &[key, newData] : updates) {
        cachedValues[key] = newData;
        executeChangedCallbacksForDataref(key.c_str());
    }
}

XPLMDataRef Dataref::findRef(const char *ref) {
    auto it = refs.find(ref);
    if (it != refs.end()) {
        return it->second;
    }

    XPLMDataRef handle = XPLMFindDataRef(ref);
    if (!handle) {
        return nullptr;
    }

    return refs.emplace(ref, handle).first->second;
}

bool Dataref::exists(const char *ref) {
    return XPLMFindDataRef(ref) != nullptr;
}

void Dataref::executeChangedCallbacksForDataref(const char *ref) {
    auto it = boundRefs.find(ref);
    if (it != boundRefs.end()) {
        for (auto callback : boundRefs[ref].changeCallbacks) {
            callback(cachedValues[ref].value);
        }
    }
}

int Dataref::getCachedLastUpdate(const char *ref) {
    auto it = cachedValues.find(ref);
    if (it == cachedValues.end()) {
        return 0;
    }

    return it->second.lastUpdateCycleNumber;
}

template float Dataref::getCached<float>(const char *ref);
template double Dataref::getCached<double>(const char *ref);
template int Dataref::getCached<int>(const char *ref);
template bool Dataref::getCached<bool>(const char *ref);
template std::vector<int> Dataref::getCached<std::vector<int>>(const char *ref);
template std::vector<float> Dataref::getCached<std::vector<float>>(const char *ref);
template std::vector<unsigned char> Dataref::getCached<std::vector<unsigned char>>(const char *ref);
template std::string Dataref::getCached<std::string>(const char *ref);

template<typename T>
T Dataref::getCached(const char *ref) {
    auto it = cachedValues.find(ref);
    if (it == cachedValues.end()) {
        auto val = get<T>(ref);
        cachedValues[ref] = {.value = val, .lastUpdateCycleNumber = XPLMGetCycleNumber()};
        return val;
    }

    if (!std::holds_alternative<T>(it->second.value)) {
        if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<int>(it->second.value)) {
                return std::get<int>(it->second.value) > 0;
            } else if (std::holds_alternative<double>(it->second.value)) {
                return std::get<double>(it->second.value) > std::numeric_limits<double>::epsilon();
            } else if (std::holds_alternative<float>(it->second.value)) {
                return std::get<float>(it->second.value) > std::numeric_limits<float>::epsilon();
            }

            return false;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                             std::is_same_v<T, std::vector<unsigned char>>) {
            return {};
        } else {
            return 0;
        }
    }

    return std::get<T>(it->second.value);
}

template float Dataref::get<float>(const char *ref);
template double Dataref::get<double>(const char *ref);
template int Dataref::get<int>(const char *ref);
template bool Dataref::get<bool>(const char *ref);
template std::vector<int> Dataref::get<std::vector<int>>(const char *ref);
template std::vector<float> Dataref::get<std::vector<float>>(const char *ref);
template std::vector<unsigned char> Dataref::get<std::vector<unsigned char>>(const char *ref);
template std::string Dataref::get<std::string>(const char *ref);

template<typename T>
T Dataref::get(const char *ref) {
    XPLMDataRef handle = findRef(ref);
    if (!handle) {
        if constexpr (std::is_same_v<T, std::string>) {
            return "";
        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                             std::is_same_v<T, std::vector<unsigned char>>) {
            return {};
        } else {
            return 0;
        }
    }

    if constexpr (std::is_same_v<T, bool>) {
        XPLMDataTypeID refType = XPLMGetDataRefTypes(handle);
        if ((refType & xplmType_Float) == xplmType_Float) {
            return XPLMGetDataf(handle) > std::numeric_limits<float>::epsilon();
        } else if ((refType & xplmType_Double) == xplmType_Double) {
            return XPLMGetDatad(handle) > std::numeric_limits<double>::epsilon();
        } else {
            return XPLMGetDatai(handle) > 0;
        }
    } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double>) {
        XPLMDataTypeID refType = XPLMGetDataRefTypes(handle);
        if ((refType & xplmType_Float) == xplmType_Float) {
            return XPLMGetDataf(handle);
        } else if ((refType & xplmType_Double) == xplmType_Double) {
            return XPLMGetDatad(handle);
        } else {
            return XPLMGetDatai(handle);
        }
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        int size = XPLMGetDatavi(handle, nullptr, 0, 0);
        std::vector<int> outValues(size);
        XPLMGetDatavi(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        int size = XPLMGetDatavf(handle, nullptr, 0, 0);
        std::vector<float> outValues(size);
        XPLMGetDatavf(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        int size = XPLMGetDatab(handle, nullptr, 0, 0);
        std::vector<unsigned char> outValues(size);
        XPLMGetDatab(handle, outValues.data(), 0, size);
        return outValues;
    } else if constexpr (std::is_same_v<T, std::string>) {
        int size = XPLMGetDatab(handle, nullptr, 0, 0);
        std::vector<char> str(size);
        XPLMGetDatab(handle, str.data(), 0, size);
        std::string out = std::string(str.data(), size);
        out.erase(std::remove(out.begin(), out.end(), '\0'), out.end());
        return out;
    }

    if constexpr (std::is_same_v<T, std::string>) {
        return "";
    } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<float>> ||
                         std::is_same_v<T, std::vector<unsigned char>>) {
        return {};
    } else {
        return 0;
    }
}

template void Dataref::set<float>(const char *ref, float value, bool setCacheOnly);
template void Dataref::set<double>(const char *ref, double value, bool setCacheOnly);
template void Dataref::set<int>(const char *ref, int value, bool setCacheOnly);
template void Dataref::set<bool>(const char *ref, bool value, bool setCacheOnly);
template void Dataref::set<std::vector<int>>(const char *ref, std::vector<int> value, bool setCacheOnly);
template void Dataref::set<std::vector<float>>(const char *ref, std::vector<float> value, bool setCacheOnly);
template void Dataref::set<std::vector<unsigned char>>(
    const char *ref, std::vector<unsigned char> value, bool setCacheOnly);
template void Dataref::set<std::string>(const char *ref, std::string value, bool setCacheOnly);

template<typename T>
void Dataref::set(const char *ref, T value, bool setCacheOnly) {
    XPLMDataRef handle = findRef(ref);
    if (!handle) {
        return;
    }

    cachedValues[ref] = {.value = value, .lastUpdateCycleNumber = XPLMGetCycleNumber()};

    executeChangedCallbacksForDataref(ref);

    if (setCacheOnly) {
        return;
    }

    if constexpr (std::is_same_v<T, bool> || std::is_same_v<T, int> || std::is_same_v<T, float> ||
                  std::is_same_v<T, double>) {
        XPLMDataTypeID refType = XPLMGetDataRefTypes(handle);
        if ((refType & xplmType_Float) == xplmType_Float) {
            XPLMSetDataf(handle, value);
        } else if ((refType & xplmType_Double) == xplmType_Double) {
            XPLMSetDatad(handle, value);
        } else {
            XPLMSetDatai(handle, value);
        }
    } else if constexpr (std::is_same_v<T, std::vector<int>>) {
        XPLMSetDatavi(handle, const_cast<int *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::vector<float>>) {
        XPLMSetDatavf(handle, const_cast<float *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::vector<unsigned char>>) {
        XPLMSetDatab(handle, const_cast<unsigned char *>(value.data()), 0, static_cast<int>(value.size()));
    } else if constexpr (std::is_same_v<T, std::string>) {
        XPLMSetDatab(handle, (char *) value.c_str(), 0, (unsigned int) value.length());
    }
}

void Dataref::executeCommand(const char *command, XPLMCommandPhase phase) {
    XPLMCommandRef handle = XPLMFindCommand(command);
    if (!handle) {
        Logger::getInstance()->info("Command not found: %s\n", command);
        return;
    }

    if (phase == -1) {
        XPLMCommandOnce(handle);
    } else if (phase == xplm_CommandBegin) {
        XPLMCommandBegin(handle);
    } else if (phase == xplm_CommandEnd) {
        XPLMCommandEnd(handle);
    }
}

void Dataref::bindExistingCommand(const char *command, CommandExecutedCallback callback) {
    XPLMCommandRef handle = XPLMFindCommand(command);
    if (!handle) {
        return;
    }

    boundCommands[command] = {handle, callback};

    XPLMRegisterCommandHandler(handle, handleCommandCallback, 1, nullptr);
}

void Dataref::createCommand(const char *command, const char *description, CommandExecutedCallback callback) {
    XPLMCommandRef handle = XPLMCreateCommand(command, description);
    if (!handle) {
        return;
    }

    auto it = boundCommands.find(command);
    if (it != boundCommands.end()) {
        XPLMUnregisterCommandHandler(handle, handleCommandCallback, 1, nullptr);
    }

    bindExistingCommand(command, callback);
}

int Dataref::_commandCallback(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void *inRefcon) {
    for (const auto &entry : boundCommands) {
        XPLMCommandRef handle = entry.second.handle;
        if (inCommand == handle) {
            entry.second.callback(inPhase);
            break;
        }
    }

    return 1;
}

#include "ffsharedvalues.h"

#include "logger.hpp"

#include <algorithm>

FFSharedValues *FFSharedValues::instance = nullptr;

FFSharedValues *FFSharedValues::getInstance() {
    if (!instance) {
        instance = new FFSharedValues();
    }

    return instance;
}

// Only a successful retain takes a reference, so a caller may keep retrying
// while the aircraft's plugin comes up without unbalancing the count.
bool FFSharedValues::retain(const char *pluginSignature) {
    if (isConnected()) {
        retainCount++;
        return true;
    }

    XPLMPluginID plugin = XPLMFindPluginBySignature(pluginSignature);
    if (plugin == XPLM_NO_PLUGIN_ID) {
        return false;
    }

    FFSharedValuesInterface received = {};
    XPLMSendMessageToPlugin(plugin, FFSharedMsgGetInterface, &received);

    // The aircraft fills the struct in place, so an untouched ValuesCount means
    // it did not answer: its plugin is loaded but not yet running the aircraft.
    if (!received.ValuesCount || !received.ValueGet || !received.ValueType) {
        return false;
    }

    iface = received;
    retainCount++;
    indexBuilt = false;
    indexedPaths.clear();
    resolvedIds.clear();
    indexedDataVersion = iface.DataVersion ? iface.DataVersion() : 0;
    indexedValuesCount = iface.ValuesCount();

    if (iface.DataAddUpdate) {
        iface.DataAddUpdate(&FFSharedValues::updateTrampoline, this);
        updateRegistered = true;
    }

    Logger::getInstance()->info("Connected to the %s shared value interface (%u values).\n", pluginSignature, iface.ValuesCount());

    return true;
}

void FFSharedValues::release() {
    if (retainCount > 0) {
        retainCount--;
    }

    if (retainCount == 0) {
        disconnect();
    }
}

void FFSharedValues::disconnect() {
    if (updateRegistered && iface.DataDelUpdate) {
        iface.DataDelUpdate(&FFSharedValues::updateTrampoline, this);
    }

    updateRegistered = false;
    iface = {};
    indexBuilt = false;
    indexedDataVersion = 0;
    indexedValuesCount = 0;
    indexedPaths.clear();
    resolvedIds.clear();
    updateCallbacks.clear();
}

void FFSharedValues::addUpdateCallback(void *owner, UpdateCallback callback) {
    removeUpdateCallback(owner);
    updateCallbacks.emplace_back(owner, std::move(callback));
}

void FFSharedValues::removeUpdateCallback(void *owner) {
    updateCallbacks.erase(std::remove_if(updateCallbacks.begin(),
                              updateCallbacks.end(),
                              [owner](const std::pair<void *, UpdateCallback> &entry) {
                                  return entry.first == owner;
                              }),
        updateCallbacks.end());
}

void FF_SHARED_CALL FFSharedValues::updateTrampoline(double step, void *tag) {
    FFSharedValues *self = static_cast<FFSharedValues *>(tag);
    if (!self) {
        return;
    }

    // Copied: a callback may add or remove entries while iterating.
    auto callbacks = self->updateCallbacks;
    for (auto &[owner, callback] : callbacks) {
        if (callback) {
            callback(step);
        }
    }
}

// The aircraft rebuilds its tree on a state change, which invalidates every id
// we cached. DataVersion is its own marker for that; the value count is checked
// as well because a tree that only grows leaves the version alone.
void FFSharedValues::dropStaleIndex() {
    unsigned int version = iface.DataVersion ? iface.DataVersion() : 0;
    unsigned int count = iface.ValuesCount ? iface.ValuesCount() : 0;

    if (version == indexedDataVersion && count == indexedValuesCount) {
        return;
    }

    indexedDataVersion = version;
    indexedValuesCount = count;
    indexBuilt = false;
    indexedPaths.clear();
    resolvedIds.clear();
}

std::string FFSharedValues::pathForId(int id, std::unordered_map<int, std::string> &memo, int depth) {
    if (id < 0 || depth > 32) {
        return {};
    }

    auto cached = memo.find(id);
    if (cached != memo.end()) {
        return cached->second;
    }

    const char *name = iface.ValueName ? iface.ValueName(id) : nullptr;
    if (!name || !*name) {
        return {};
    }

    std::string path = name;
    int parent = iface.ValueParent ? iface.ValueParent(id) : -1;
    if (parent >= 0 && parent != id) {
        std::string parentPath = pathForId(parent, memo, depth + 1);
        if (!parentPath.empty()) {
            path = parentPath + "." + path;
        }
    }

    memo.emplace(id, path);

    return path;
}

void FFSharedValues::buildIndex() {
    if (indexBuilt || !iface.ValuesCount || !iface.ValueIdByIndex) {
        return;
    }

    indexBuilt = true;

    std::unordered_map<int, std::string> memo;
    unsigned int count = iface.ValuesCount();

    for (unsigned int index = 0; index < count; ++index) {
        int id = iface.ValueIdByIndex(index);
        if (id < 0) {
            continue;
        }

        std::string path = pathForId(id, memo);
        if (!path.empty()) {
            indexedPaths[path] = id;
        }
    }

    Logger::getInstance()->debug("Indexed %zu FlightFactor shared values.\n", indexedPaths.size());
}

int FFSharedValues::idForPath(const std::string &path) {
    if (!isConnected()) {
        return -1;
    }

    dropStaleIndex();

    auto cached = resolvedIds.find(path);
    if (cached != resolvedIds.end()) {
        return cached->second;
    }

    int id = -1;

    if (iface.ValueIdByName) {
        id = iface.ValueIdByName(path.c_str());
    }

    if (id < 0) {
        buildIndex();

        auto exact = indexedPaths.find(path);
        if (exact != indexedPaths.end()) {
            id = exact->second;
        } else {
            // The aircraft roots its tree under an object of its own ("a320"),
            // which the web MCDU leaves out of its paths, so match on the tail.
            const std::string suffix = "." + path;
            for (const auto &[indexedPath, indexedId] : indexedPaths) {
                if (indexedPath.size() > suffix.size() && indexedPath.compare(indexedPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
                    id = indexedId;
                    break;
                }
            }
        }
    }

    if (id < 0) {
        Logger::getInstance()->debug("FlightFactor shared value \"%s\" not found.\n", path.c_str());

        // A miss before the tree is fully published is not final, so it is not
        // cached: the aircraft adds its objects as its systems come up.
        if (!indexBuilt) {
            return id;
        }
    }

    resolvedIds[path] = id;

    return id;
}

std::string FFSharedValues::getString(const std::string &path) {
    int id = idForPath(path);
    if (id < 0 || !iface.ValueGet || !iface.ValueGetSize) {
        return {};
    }

    unsigned int size = iface.ValueGetSize(id);
    if (!size || size > 4096) {
        return {};
    }

    // Reused across calls: the MCDU display alone reads 28 of these per frame.
    // Main thread only, like every other call here.
    static std::vector<char> buffer;
    buffer.assign(size + 1, 0);
    iface.ValueGet(id, buffer.data());
    buffer[size] = '\0';

    return std::string(buffer.data());
}

double FFSharedValues::getNumber(const std::string &path) {
    int id = idForPath(path);
    if (id < 0 || !iface.ValueGet || !iface.ValueType) {
        return 0.0;
    }

    switch (iface.ValueType(id)) {
        case FFValueTypeSint8: {
            signed char value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeUint8: {
            unsigned char value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeSint16: {
            short value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeUint16: {
            unsigned short value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeSint32: {
            int value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeUint32: {
            unsigned int value = 0;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeFloat32: {
            float value = 0.0f;
            iface.ValueGet(id, &value);
            return value;
        }

        case FFValueTypeFloat64:
        case FFValueTypeTime: {
            double value = 0.0;
            iface.ValueGet(id, &value);
            return value;
        }

        default:
            return 0.0;
    }
}

void FFSharedValues::setNumber(const std::string &path, double value) {
    int id = idForPath(path);
    if (id < 0 || !iface.ValueSet || !iface.ValueType) {
        return;
    }

    // The width has to match what the aircraft declared: ValueSet reads straight
    // through the pointer.
    switch (iface.ValueType(id)) {
        case FFValueTypeSint8: {
            signed char target = static_cast<signed char>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeUint8: {
            unsigned char target = static_cast<unsigned char>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeSint16: {
            short target = static_cast<short>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeUint16: {
            unsigned short target = static_cast<unsigned short>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeSint32: {
            int target = static_cast<int>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeUint32: {
            unsigned int target = static_cast<unsigned int>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeFloat32: {
            float target = static_cast<float>(value);
            iface.ValueSet(id, &target);
            break;
        }

        case FFValueTypeFloat64:
        case FFValueTypeTime: {
            double target = value;
            iface.ValueSet(id, &target);
            break;
        }

        default:
            break;
    }
}

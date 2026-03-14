#include <cstdio>
#include <algorithm>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>
#include <XPLMDisplay.h>
#include <XPLMDataAccess.h>
#include <XPLMMenus.h>
#include "dataref.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <variant>
#include <cstring>
#include <ctime>

// Forward declarations for XPLM types (they are defined as void* in the actual headers)
typedef void* XPLMCommandRef;
typedef void* XPLMDataRef;
typedef int XPLMDataTypeID;
typedef int (*XPLMCommandCallback_f)(XPLMCommandRef inCommand, int inPhase, void *inRefcon);

// Data type constants matching XPLM
#define xplmType_Unknown       0
#define xplmType_Int           1
#define xplmType_Float         2
#define xplmType_Double        4
#define xplmType_FloatArray    8
#define xplmType_IntArray      16
#define xplmType_Data          32

// Mock dataref storage using a union-like approach
struct MockDataRef {
    std::string name;
    XPLMDataTypeID type;
    
    // Storage for different data types
    int intValue;
    float floatValue;
    double doubleValue;
    std::vector<int> intArrayValue;
    std::vector<float> floatArrayValue;
    std::vector<unsigned char> dataValue;
    
    MockDataRef(const std::string& n, XPLMDataTypeID t) : name(n), type(t), intValue(0), floatValue(0.0f), doubleValue(0.0) {}
};

// Helper function to create a dataref handle from an index
XPLMDataRef indexToDataRefHandle(size_t index) {
    return reinterpret_cast<XPLMDataRef>(index + 1000); // Offset to avoid confusion with command handles
}

// Helper function to get index from dataref handle
size_t dataRefHandleToIndex(XPLMDataRef ref) {
    return reinterpret_cast<size_t>(ref) - 1000;
}

XPLMMenuID mainMenuId = 0;
static std::vector<std::string> registeredCommands = {};
static std::vector<MockDataRef> mockDataRefs = {};
static std::unordered_map<std::string, size_t> dataRefNameToIndex = {};

// Function to clear all mock dataref storage
void clearAllMockDataRefs() {
    registeredCommands.clear();
    mockDataRefs.clear();
    dataRefNameToIndex.clear();
}

// Utility function to create a dataref with a specific type (for testing purposes)
XPLMDataRef createMockDataRef(const char* name, XPLMDataTypeID type) {
    std::string nameStr(name);
    
    // Check if it already exists
    auto it = dataRefNameToIndex.find(nameStr);
    if (it != dataRefNameToIndex.end()) {
        // Update the type if it already exists
        mockDataRefs[it->second].type = type;
        return indexToDataRefHandle(it->second);
    }
    
    mockDataRefs.emplace_back(nameStr, type);
    size_t index = mockDataRefs.size() - 1;
    dataRefNameToIndex[nameStr] = index;
    
    return indexToDataRefHandle(index);
}

// Helper function to create a dataref with type inference based on name patterns
XPLMDataRef createMockDataRefWithInference(const char* name, XPLMDataTypeID preferredType) {
    std::string nameStr(name);
    
    // Start with the preferred type
    XPLMDataTypeID defaultType = preferredType;
    
    // Try to infer type from common dataref patterns if no preferred type
    if (defaultType == xplmType_Unknown) {
        if (nameStr.find("_int") != std::string::npos || 
            nameStr.find("Count") != std::string::npos ||
            nameStr.find("_enum") != std::string::npos ||
            nameStr.find("_bool") != std::string::npos) {
            defaultType = xplmType_Int;
        } else if (nameStr.find("Array") != std::string::npos || 
                   nameStr.find("array") != std::string::npos ||
                   nameStr.find("DUBrightness") != std::string::npos) {
            defaultType = xplmType_FloatArray;
        } else if (nameStr.find("_double") != std::string::npos) {
            defaultType = xplmType_Double;
        } else {
            defaultType = xplmType_Float; // Default fallback
        }
    }
    
    // Special handling for known datarefs
    if (nameStr == "AirbusFBW/DUBrightness") {
        defaultType = xplmType_FloatArray;
    } else if (nameStr == "AirbusFBW/PanelBrightnessLevel") {
        defaultType = xplmType_Float;
    }
    
    return createMockDataRef(name, defaultType);
}

// Helper function to get or create a dataref from a handle, creating it if it doesn't exist
MockDataRef* getOrCreateDataRefFromHandle(XPLMDataRef ref, XPLMDataTypeID preferredType = xplmType_Unknown) {
    if (!ref) return nullptr;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index < mockDataRefs.size()) {
        return &mockDataRefs[index];
    }
    
    // If we get here, the handle is invalid - this shouldn't happen in normal operation
    return nullptr;
}

// Helper function to get or create a dataref by name for setters
MockDataRef* getOrCreateDataRefByName(const char* name, XPLMDataTypeID preferredType) {
    std::string nameStr(name);
    
    // Check if it already exists
    auto it = dataRefNameToIndex.find(nameStr);
    if (it != dataRefNameToIndex.end()) {
        return &mockDataRefs[it->second];
    }
    
    // Create it with the preferred type
    XPLMDataRef ref = createMockDataRefWithInference(name, preferredType);
    size_t index = dataRefHandleToIndex(ref);
    return &mockDataRefs[index];
}

void XPLMCommandBegin(XPLMCommandRef ref) {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(ref)) - 1;
    if (idx >= 0 && idx < static_cast<int>(registeredCommands.size())) {
        printf("Executing command (start): %s\n", registeredCommands[idx].c_str());
    } else {
        printf("Executing command (start): invalid ref\n");
    }
}

void XPLMCommandEnd(XPLMCommandRef ref) {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(ref)) - 1;
    if (idx >= 0 && idx < static_cast<int>(registeredCommands.size())) {
        printf("Executing command (end): %s\n", registeredCommands[idx].c_str());
    } else {
        printf("Executing command (end): invalid ref\n");
    }

    if (registeredCommands[idx] == "AirbusFBW/MCDU1KeyBright" || registeredCommands[idx] == "AirbusFBW/MCDU1KeyDim") {
        float brightness = registeredCommands[idx] == "AirbusFBW/MCDU1KeyDim" ? 0.2 : 0.8;
        Dataref::getInstance()->set<float>("AirbusFBW/PanelBrightnessLevel", brightness, true);
        Dataref::getInstance()->set<std::vector<float>>("AirbusFBW/DUBrightness", {brightness, brightness, brightness, brightness, brightness, brightness, brightness, brightness}, true);
    }
}

void XPLMCommandOnce(XPLMCommandRef ref) {
    int idx = static_cast<int>(reinterpret_cast<intptr_t>(ref)) - 1;
    if (idx >= 0 && idx < static_cast<int>(registeredCommands.size())) {
        printf("Executing command (once): %s\n", registeredCommands[idx].c_str());
    } else {
        printf("Executing command (once): invalid ref\n");
    }
}

XPLMCommandRef XPLMCreateCommand(const char *name, const char *desc) {
    printf("Creating command: %s\n", name);
    return 0;
}

void XPLMDebugString(const char *data) {
    printf("%s", data);
}

XPLMCommandRef XPLMFindCommand(const char *name) {
    std::string nameStr(name);
    auto it = std::find(registeredCommands.begin(), registeredCommands.end(), nameStr);
    if (it != registeredCommands.end()) {
        return (XPLMCommandRef)(std::distance(registeredCommands.begin(), it) + 1);
    }
    registeredCommands.push_back(nameStr);
    return (XPLMCommandRef)registeredCommands.size();
}

XPLMDataRef XPLMFindDataRef(const char *name) {
    std::string nameStr(name);
    
    // Check if we already have this dataref
    auto it = dataRefNameToIndex.find(nameStr);
    if (it != dataRefNameToIndex.end()) {
        return indexToDataRefHandle(it->second);
    }
    
    return nullptr;
}

XPLMDataTypeID XPLMGetDataRefTypes(XPLMDataRef ref) {
    if (!ref) return xplmType_Unknown;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) {
        return xplmType_Unknown;
    }
    
    return mockDataRefs[index].type;
}

int XPLMGetDatab(XPLMDataRef ref, void *outValue, int offset, int maxBytes) {
    if (!ref) return 0;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_Data) return 0;
    
    // If outValue is null, return total size (X-Plane SDK behavior)
    if (!outValue) {
        return static_cast<int>(dataref.dataValue.size());
    }
    
    size_t availableBytes = dataref.dataValue.size() - offset;
    size_t bytesToCopy = std::min(static_cast<size_t>(maxBytes), availableBytes);
    
    if (bytesToCopy > 0) {
        memcpy(outValue, dataref.dataValue.data() + offset, bytesToCopy);
    }
    
    return static_cast<int>(bytesToCopy);
}

float XPLMGetDataf(XPLMDataRef ref) {
    if (!ref) return 0.0f;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0.0f;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Float:
            return dataref.floatValue;
        case xplmType_Int:
            return static_cast<float>(dataref.intValue);
        case xplmType_Double:
            return static_cast<float>(dataref.doubleValue);
        default:
            return 0.0f;
    }
}

double XPLMGetDatad(XPLMDataRef ref) {
    if (!ref) return 0.0;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0.0;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Float:
            return dataref.floatValue;
        case xplmType_Int:
            return static_cast<double>(dataref.intValue);
        case xplmType_Double:
            return static_cast<double>(dataref.doubleValue);
        default:
            return 0.0;
    }
}

int XPLMGetDatai(XPLMDataRef ref) {
    if (!ref) return 0;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Int:
            return dataref.intValue;
        case xplmType_Float:
            return static_cast<int>(dataref.floatValue);
        case xplmType_Double:
            return static_cast<int>(dataref.doubleValue);
        default:
            return 0;
    }
}

int XPLMGetDatavf(XPLMDataRef ref, float *values, int offset, int max) {
    if (!ref) return 0;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_FloatArray) return 0;
    
    // If values is null, return total size (X-Plane SDK behavior)
    if (!values) {
        return static_cast<int>(dataref.floatArrayValue.size());
    }
    
    size_t availableValues = dataref.floatArrayValue.size() - offset;
    size_t valuesToCopy = std::min(static_cast<size_t>(max), availableValues);
    
    if (valuesToCopy > 0) {
        memcpy(values, dataref.floatArrayValue.data() + offset, valuesToCopy * sizeof(float));
    }
    
    return static_cast<int>(valuesToCopy);
}

int XPLMGetDatavi(XPLMDataRef ref, int *values, int offset, int max) {
    if (!ref) return 0;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return 0;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_IntArray) return 0;
    
    // If values is null, return total size (X-Plane SDK behavior)
    if (!values) {
        return static_cast<int>(dataref.intArrayValue.size());
    }
    
    size_t availableValues = dataref.intArrayValue.size() - offset;
    size_t valuesToCopy = std::min(static_cast<size_t>(max), availableValues);
    
    if (valuesToCopy > 0) {
        memcpy(values, dataref.intArrayValue.data() + offset, valuesToCopy * sizeof(int));
    }
    
    return static_cast<int>(valuesToCopy);
}

void XPLMRegisterCommandHandler(XPLMDataRef ref, XPLMCommandCallback_f handler, int before, void *refcon) {
}

// Mock implementations for missing XPLM functions

void XPLMUnregisterCommandHandler(XPLMDataRef ref, XPLMCommandCallback_f handler, int before, void *refcon) {
    printf("Unregistering command handler\n");
}

void XPLMUnregisterDataAccessor(XPLMDataRef ref) {
    printf("Unregistering data accessor\n");
}

void XPLMSetDatai(XPLMDataRef ref, int value) {
    if (!ref) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Int:
            dataref.intValue = value;
            break;
        case xplmType_Float:
            dataref.floatValue = static_cast<float>(value);
            break;
        case xplmType_Double:
            dataref.doubleValue = static_cast<double>(value);
            break;
        default:
            printf("Cannot set int value on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
            break;
    }
}

void XPLMSetDataf(XPLMDataRef ref, float value) {
    if (!ref) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Float:
            dataref.floatValue = value;
            break;
        case xplmType_Int:
            dataref.intValue = static_cast<int>(value);
            break;
        case xplmType_Double:
            dataref.doubleValue = static_cast<double>(value);
            break;
        default:
            printf("Cannot set float value on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
            break;
    }
}

void XPLMSetDatad(XPLMDataRef ref, double value) {
    if (!ref) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    switch (dataref.type) {
        case xplmType_Float:
            dataref.floatValue = value;
            break;
        case xplmType_Int:
            dataref.intValue = static_cast<int>(value);
            break;
        case xplmType_Double:
            dataref.doubleValue = static_cast<double>(value);
            break;
        default:
            printf("Cannot set float value on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
            break;
    }
}

void XPLMSetDatab(XPLMDataRef ref, void *inValue, int inOffset, int inLength) {
    if (!ref || !inValue) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_Data) {
        printf("Cannot set data bytes on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
        return;
    }
    
    // Resize if necessary
    size_t requiredSize = inOffset + inLength;
    if (dataref.dataValue.size() < requiredSize) {
        dataref.dataValue.resize(requiredSize);
    }
    
    memcpy(dataref.dataValue.data() + inOffset, inValue, inLength);
}

void XPLMSetDatavi(XPLMDataRef ref, int *inValues, int inOffset, int inCount) {
    if (!ref || !inValues) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_IntArray) {
        printf("Cannot set int array on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
        return;
    }
    
    // Resize if necessary
    size_t requiredSize = inOffset + inCount;
    if (dataref.intArrayValue.size() < requiredSize) {
        dataref.intArrayValue.resize(requiredSize);
    }
    
    memcpy(dataref.intArrayValue.data() + inOffset, inValues, inCount * sizeof(int));
}

void XPLMSetDatavf(XPLMDataRef ref, float *inValues, int inOffset, int inCount) {
    if (!ref || !inValues) return;
    
    size_t index = dataRefHandleToIndex(ref);
    if (index >= mockDataRefs.size()) return;
    
    MockDataRef& dataref = mockDataRefs[index];
    if (dataref.type != xplmType_FloatArray) {
        printf("Cannot set float array on dataref '%s' of type %d\n", dataref.name.c_str(), dataref.type);
        return;
    }
    
    // Resize if necessary
    size_t requiredSize = inOffset + inCount;
    if (dataref.floatArrayValue.size() < requiredSize) {
        dataref.floatArrayValue.resize(requiredSize);
    }
    
    memcpy(dataref.floatArrayValue.data() + inOffset, inValues, inCount * sizeof(float));
}

XPLMDataRef XPLMRegisterDataAccessor(
    const char *inDataName,
    XPLMDataTypeID inDataType,
    int inIsWritable,
    XPLMGetDatai_f inReadInt,
    XPLMSetDatai_f inWriteInt,
    XPLMGetDataf_f inReadFloat,
    XPLMSetDataf_f inWriteFloat,
    XPLMGetDatad_f inReadDouble,
    XPLMSetDatad_f inWriteDouble,
    XPLMGetDatavi_f inReadIntArray,
    XPLMSetDatavi_f inWriteIntArray,
    XPLMGetDatavf_f inReadFloatArray,
    XPLMSetDatavf_f inWriteFloatArray,
    XPLMGetDatab_f inReadData,
    XPLMSetDatab_f inWriteData,
    void *inReadRefcon,
    void *inWriteRefcon)
{
    printf("Registering data accessor: %s\n", inDataName);
    return 0;
}

void XPLMRegisterFlightLoopCallback(XPLMFlightLoop_f inFlightLoop, float inInterval, void *inRefcon) {
    
}

void XPLMUnregisterFlightLoopCallback(XPLMFlightLoop_f inFlightLoop, void *inRefcon) {
    
}

int XPLMGetCycleNumber() {
    return static_cast<int>(std::time(nullptr));
}

XPLMMenuID XPLMCreateMenu(const char *inName, XPLMMenuID inParentMenu, int inParentItem, XPLMMenuHandler_f inHandler, void *inMenuRef) {
    return mainMenuId;
}

int XPLMAppendMenuItem(XPLMMenuID inMenu, const char *inItemName, void *inItemRef, int inDeprecatedAndIgnored) {
    return 0;
}

void XPLMCheckMenuItem(XPLMMenuID inMenu, int index, XPLMMenuCheck inCheck) {
    // noop
}

void XPLMGetSystemPath(char *outSystemPath) {
    // noop
}

void XPLMCheckMenuItemState(XPLMMenuID inMenu, int inIndex, XPLMMenuCheck *out) {
    // noop
}

void XPLMClearAllMenuItems(XPLMMenuID inMenu) {
    // noop
}

void XPLMDestroyMenu(XPLMMenuID inMenu) {
    // noop
}

XPLMMenuID XPLMFindPluginsMenu() {
    // noop
    return 0;
}

void XPLMRemoveMenuItem(XPLMMenuID inMenu, int inIndex) {
    // noop
}

void XPLMSetMenuItemName(XPLMMenuID inMenu, int inIndex, const char *inItemName, int dep) {
    // noop
}

void XPLMAppendMenuSeparator(XPLMMenuID inMenu) {
    // noop
}

float XPLMGetElapsedTime() {
    return 1.0f;
}

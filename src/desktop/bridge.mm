#include "bridge.h"
#include "usbcontroller.h"
#include "appstate.h"
#include "dataref.h"
#include "product-joystick.h"
#include "product-fmc.h"
#include "product-fcu-efis.h"
#include "font.h"
#include <vector>
#include <string>
#include <cstring>

// Forward declaration for mock dataref creation function
typedef void* XPLMDataRef;
typedef int XPLMDataTypeID;
#define xplmType_Data 32
#define xplmType_Float 2
#define xplmType_Int 1
#define xplmType_FloatArray 8
#define xplmType_IntArray 16

XPLMDataRef XPLMFindDataRef(const char* name);
XPLMDataRef createMockDataRefWithInference(const char* name, XPLMDataTypeID preferredType);
void clearAllMockDataRefs();

typedef int XPLMHostApplicationID;

void XPLMGetVersions(int* outXPlaneVersion, int* outXPLMVersion, XPLMHostApplicationID* outHostID) {
    if (outXPlaneVersion) *outXPlaneVersion = 12000;
    if (outXPLMVersion)   *outXPLMVersion   = 400;
    if (outHostID)        *outHostID         = 1;
}


// Helper function to ensure dataref exists before setting
void ensureDatarefExists(const char* ref, XPLMDataTypeID preferredType) {
    if (!XPLMFindDataRef(ref)) {
        createMockDataRefWithInference(ref, preferredType);
    }
}

void clearDatarefCache() {
    Dataref::getInstance()->clearCache();
    clearAllMockDataRefs();
}

void setDatarefHexC(const char* ref, const uint8_t* hexD, int len) {
    const std::vector<uint8_t>& hex = std::vector<uint8_t>(hexD, hexD + len);
    
    // Check if this is a style dataref - if so, store as vector<unsigned char>
    std::string refStr(ref);
    if (refStr.find("style_line") != std::string::npos || refStr.find("ixeg/") != std::string::npos || refStr.find("XCrafts/") != std::string::npos) {
        // Ensure dataref exists first
        ensureDatarefExists(ref, xplmType_Data);
        
        std::vector<unsigned char> styleBytes;
        for (uint8_t c : hex) {
            styleBytes.push_back(c);
        }
        Dataref::getInstance()->set<std::vector<unsigned char>>(ref, styleBytes, false);
    } else {
        // Ensure dataref exists first
        ensureDatarefExists(ref, xplmType_Data);
        
        // For text datarefs, convert to string (stopping at null terminator)
        std::string s;
        for (uint8_t c : hex) {
            if (c == 0x00) break;
            s += static_cast<char>(c);
        }
        Dataref::getInstance()->set<std::string>(ref, s, false);
    }
}

void setDatarefFloat(const char* ref, float value) {
    // Ensure dataref exists first
    ensureDatarefExists(ref, xplmType_Float);
    
    Dataref::getInstance()->set<float>(ref, value, false);
}

void setDatarefInt(const char* ref, int value) {
    // Ensure dataref exists first
    ensureDatarefExists(ref, xplmType_Int);
    
    Dataref::getInstance()->set<int>(ref, value, false);
}

void setDatarefFloatVector(const char* ref, const float* values, int count) {
    // Ensure dataref exists first
    ensureDatarefExists(ref, xplmType_FloatArray);
    
    std::vector<float> floatVector(values, values + count);
    Dataref::getInstance()->set<std::vector<float>>(ref, floatVector, false);
}

void setDatarefFloatVectorRepeated(const char* ref, float value, int count) {
    // Ensure dataref exists first
    ensureDatarefExists(ref, xplmType_FloatArray);
    
    std::vector<float> floatVector(count, value);
    Dataref::getInstance()->set<std::vector<float>>(ref, floatVector, false);
}

void setDatarefIntVector(const char* ref, const int* values, int count) {
    // Ensure dataref exists first
    ensureDatarefExists(ref, xplmType_IntArray);
    
    std::vector<int> intVector(values, values + count);
    Dataref::getInstance()->set<std::vector<int>>(ref, intVector, false);
}

void update() {
    AppState::getInstance()->pluginInitialized = true;
    AppState::Update(0.0f, 0.0f, 1, nullptr);
}

void disconnectAll() {
    for (const auto& device : USBController::getInstance()->devices) {
        device->disconnect();
    }
}

int enumerateDevices(char *buffer, int bufferLen) {
    //USBController::getInstance()->reloadDevices();
    
    int count = 0;
    std::string result;
    for (const auto& device : USBController::getInstance()->devices) {
        if (!result.empty()) result += "\n";
        result += device->productName;
        count++;
    }
    if ((int)result.size() + 1 > bufferLen) {
        // Not enough space in buffer
        return -1;
    }
    std::strncpy(buffer, result.c_str(), bufferLen);
    buffer[bufferLen-1] = '\0';
    return count;
}

// Device handle access functions
void* getDeviceHandle(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    return devices[deviceIndex];
}

void* getJoystickHandle(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    return dynamic_cast<ProductJoystick*>(devices[deviceIndex]);
}

void* getFMCHandle(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    return dynamic_cast<ProductFMC*>(devices[deviceIndex]);
}

void* getFCUEfisHandle(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    return dynamic_cast<ProductFCUEfis*>(devices[deviceIndex]);
}

// Generic device functions via handle
bool device_connect(void* deviceHandle) {
    if (!deviceHandle) return false;
    auto device = static_cast<USBDevice*>(deviceHandle);
    return device->connect();
}

void device_disconnect(void* deviceHandle) {
    if (!deviceHandle) return;
    auto device = static_cast<USBDevice*>(deviceHandle);
    device->disconnect();
}

void device_update(void* deviceHandle) {
    if (!deviceHandle) return;
    auto device = static_cast<USBDevice*>(deviceHandle);
    device->update();
}

void device_force_state_sync(void* deviceHandle) {
    if (!deviceHandle) return;
    auto device = static_cast<USBDevice*>(deviceHandle);
    device->forceStateSync();
}

// Joystick functions via handle
void joystick_setVibration(void* joystickHandle, uint8_t vibration) {
    if (!joystickHandle) return;
    auto joystick = static_cast<ProductJoystick*>(joystickHandle);
    joystick->setVibration(vibration);
}

void joystick_setLedBrightness(void* joystickHandle, uint8_t brightness) {
    if (!joystickHandle) return;
    auto joystick = static_cast<ProductJoystick*>(joystickHandle);
    joystick->setLedBrightness(brightness);
}

// FMC functions via handle
void fmc_showBackground(void* fmcHandle, int variant) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->showBackground((FMCBackgroundVariant)variant);
}

bool fmc_setLed(void* fmcHandle, int ledId, uint8_t value) {
    if (!fmcHandle) return false;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->setLedBrightness(static_cast<FMCLed>(ledId), value);
    return true;
}

// Additional FMC functions via handle
void fmc_clearDisplay(void* fmcHandle) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->clearDisplay();
}

void fmc_unloadProfile(void* fmcHandle) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->unloadProfile();
}

void fmc_setLedBrightness(void* fmcHandle, int ledId, uint8_t brightness) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->setLedBrightness(static_cast<FMCLed>(ledId), brightness);
}

bool fmc_writeData(void* fmcHandle, const uint8_t* data, int length) {
    if (!fmcHandle || !data || length <= 0) return false;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    std::vector<uint8_t> dataVector(data, data + length);
    return fmc->writeData(dataVector);
}

static FontVariant fontVariantFromType(int fontType) {
    switch (fontType) {
        case 1: // Airbus
            return FontVariant::FontAirbus;
        case 2: // 737
            return FontVariant::Font737;
        case 3: // X-Crafts
            return FontVariant::FontXCrafts;
        case 4: // VGA 1
            return FontVariant::FontVGA1;

        case 0:
        default:
            return FontVariant::Default;
    }
}

void fmc_setFont(void* fmcHandle, int fontType) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->setFont(fontVariantFromType(fontType));
}

void fmc_setScreenLayout(void* fmcHandle, int fontType, int characterHeight, int characterWidth, int x, int y) {
    if (!fmcHandle) return;
    auto fmc = static_cast<ProductFMC*>(fmcHandle);
    fmc->setScreenLayout(fontVariantFromType(fontType), static_cast<unsigned char>(characterHeight), static_cast<unsigned char>(characterWidth), static_cast<unsigned char>(x), static_cast<unsigned char>(y));
}

// Device enumeration and info functions
int getDeviceCount() {
    return static_cast<int>(USBController::getInstance()->devices.size());
}

const char* getDeviceName(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    return devices[deviceIndex]->productName.c_str();
}

const char* getDeviceType(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return nullptr;
    }
    
    auto device = devices[deviceIndex];
    if (dynamic_cast<ProductJoystick*>(device)) {
        return "joystick";
    } else if (dynamic_cast<ProductFMC*>(device)) {
        return "fmc";
    } else if (dynamic_cast<ProductFCUEfis*>(device)) {
        return "fcu-efis";
    }
    return "unknown";
}

uint16_t getDeviceProductId(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return 0;
    }
    return devices[deviceIndex]->productId;
}

bool isDeviceConnected(int deviceIndex) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return false;
    }
    return devices[deviceIndex]->connected;
}

// Joystick-specific functions
void joystick_setVibration(int deviceIndex, uint8_t vibration) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return;
    }
    
    auto joystick = dynamic_cast<ProductJoystick*>(devices[deviceIndex]);
    if (joystick) {
        joystick->setVibration(vibration);
    }
}

void joystick_setLedBrightness(int deviceIndex, uint8_t brightness) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return;
    }
    
    auto joystick = dynamic_cast<ProductJoystick*>(devices[deviceIndex]);
    if (joystick) {
        joystick->setLedBrightness(brightness);
    }
}

// FMC-specific functions
bool fmc_clearDisplay(int deviceIndex, int displayId) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return false;
    }
    
    auto fmc = dynamic_cast<ProductFMC*>(devices[deviceIndex]);
    if (fmc) {
        fmc->showBackground((FMCBackgroundVariant)displayId);
        return true;
    }
    return false;
}

bool fmc_setBacklight(int deviceIndex, uint8_t brightness) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return false;
    }
    
    auto fmc = dynamic_cast<ProductFMC*>(devices[deviceIndex]);
    if (fmc) {
        fmc->setLedBrightness(FMCLed::BACKLIGHT, brightness);
        return true;
    }
    return false;
}

bool fmc_setScreenBacklight(int deviceIndex, uint8_t brightness) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return false;
    }
    
    auto fmc = dynamic_cast<ProductFMC*>(devices[deviceIndex]);
    if (fmc) {
        fmc->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, brightness);
        return true;
    }
    return false;
}

bool fmc_setLed(int deviceIndex, int ledId, bool state) {
    auto& devices = USBController::getInstance()->devices;
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(devices.size())) {
        return false;
    }
    
    auto fmc = dynamic_cast<ProductFMC*>(devices[deviceIndex]);
    if (fmc) {
        fmc->setLedBrightness(FMCLed(ledId), state ? 1 : 0);
        return true;
    }
    return false;
}

// FCU-EFIS functions via handle
void fcuefis_clear(void* fcuefisHandle) {
    if (!fcuefisHandle) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    // FCU-EFIS doesn't have a clear function like FMC
    // Instead, we can set displays to show test values or blank
    fcuefis->initializeDisplays();
}

bool fcuefis_setLed(void* fcuefisHandle, int ledId, uint8_t value) {
    if (!fcuefisHandle) return false;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    fcuefis->setLedBrightness(static_cast<FCUEfisLed>(ledId), value);
    return true;
}

void fcuefis_setLedBrightness(void* fcuefisHandle, int ledId, uint8_t brightness) {
    if (!fcuefisHandle) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    fcuefis->setLedBrightness(static_cast<FCUEfisLed>(ledId), brightness);
}

void fcuefis_testDisplay(void* fcuefisHandle, const char* testType) {
    if (!fcuefisHandle || !testType) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    
    fcuefis->sendFCUDisplay("888", "888", "88888", "8888");
    
    // Send test pattern to both EFIS displays
    EfisDisplayValue efisData;
    efisData.baro = "8888";
    efisData.unitIsInHg = false;
    efisData.showQfe = false;
    fcuefis->sendEfisDisplayWithFlags(&efisData, true);  // Right
    fcuefis->sendEfisDisplayWithFlags(&efisData, false); // Left
}

void fcuefis_efisRightTestDisplay(void* fcuefisHandle, const char* testType) {
    if (!fcuefisHandle || !testType) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    
    std::string test(testType);
    EfisDisplayValue efisData;
    
    if (test == "QNH_1013") {
        // hPa: QNH mode but no decimal point
        efisData.baro = "1013";
        efisData.unitIsInHg = false;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, true);
    } else if (test == "QNH_2992") {
        // inHg: show decimal point to display "29.92"
        efisData.baro = "2992";
        efisData.unitIsInHg = true;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, true);
    } else if (test == "STD") {
        // STD: no decimal point
        efisData.baro = "";
        efisData.isStd = true;
        efisData.unitIsInHg = false;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, true);
    }
}

void fcuefis_efisLeftTestDisplay(void* fcuefisHandle, const char* testType) {
    if (!fcuefisHandle || !testType) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    
    std::string test(testType);
    EfisDisplayValue efisData;
    
    if (test == "QNH_1013") {
        // hPa: QNH mode but no decimal point
        efisData.baro = "1013";
        efisData.unitIsInHg = false;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, false);
    } else if (test == "QNH_2992") {
        // inHg: show decimal point to display "29.92"
        efisData.baro = "2992";
        efisData.unitIsInHg = true;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, false);
    } else if (test == "STD") {
        // STD: no decimal point
        efisData.baro = "";
        efisData.isStd = true;
        efisData.unitIsInHg = false;
        efisData.showQfe = false;
        fcuefis->sendEfisDisplayWithFlags(&efisData, false);
    }
}

void fcuefis_efisRightClear(void* fcuefisHandle) {
    if (!fcuefisHandle) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    
    EfisDisplayValue efisData;
    efisData.baro = "    ";  // Clear with 4 spaces
    efisData.unitIsInHg = false;
    efisData.showQfe = false;
    fcuefis->sendEfisDisplayWithFlags(&efisData, true);
}

void fcuefis_efisLeftClear(void* fcuefisHandle) {
    if (!fcuefisHandle) return;
    auto fcuefis = static_cast<ProductFCUEfis*>(fcuefisHandle);
    
    EfisDisplayValue efisData;
    efisData.baro = "    ";  // Clear with 4 spaces
    efisData.unitIsInHg = false;
    efisData.showQfe = false;
    fcuefis->sendEfisDisplayWithFlags(&efisData, false);
}

// Button identification mode
static bool buttonListeningMode = false;
static bool buttonHasBeenPressed = false;
static int lastPressedButtonId = -1;
static int lastPressedProductId = -1;

void setButtonListeningMode(bool enabled) {
    buttonListeningMode = enabled;
    if (enabled) {
        buttonHasBeenPressed = false;
        lastPressedButtonId = -1;
        lastPressedProductId = -1;
    }
}

bool getButtonListeningMode() {
    return buttonListeningMode;
}

bool hasButtonPressed() {
    return buttonHasBeenPressed;
}

int getLastPressedButtonId() {
    return lastPressedButtonId;
}

int getLastPressedProductId() {
    return lastPressedProductId;
}

void clearLastPressedButton() {
    buttonHasBeenPressed = false;
    lastPressedButtonId = -1;
    lastPressedProductId = -1;
}

extern "C++" void notifyButtonPressed(uint16_t buttonId, uint16_t productId) {
    if (buttonListeningMode) {
        buttonHasBeenPressed = true;
        lastPressedButtonId = static_cast<int>(buttonId);
        lastPressedProductId = static_cast<int>(productId);
    }
}

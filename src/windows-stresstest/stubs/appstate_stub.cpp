// Minimal AppState implementation for the standalone Windows stress test.
// Replaces the full X-Plane-dependent appstate.cpp.
// - pluginInitialized is always true
// - executeAfter / executeAfterDebounced call the function immediately
//   (usbcontroller_win.cpp uses executeAfter(0, owner, lambda) to create devices)
// - Everything else is a no-op / sensible default

#include "appstate.h"

#include <windows.h>

AppState *AppState::instance = nullptr;

AppState::AppState() {
    pluginInitialized = true;
}

AppState::~AppState() {
    instance = nullptr;
}

AppState *AppState::getInstance() {
    if (!instance) {
        instance = new AppState();
    }
    return instance;
}

bool AppState::initialize() {
    pluginInitialized = true;
    return true;
}

void AppState::deinitialize() {
    pluginInitialized = false;
}

float AppState::Update(float, float, int, void *) {
    return 0.0f;
}

// Font::GlyphData() reads <pluginDirectory>/fonts/<file>. The harness keeps a
// copy of the repository's fonts/ next to the exe (see CMakeLists), so pointing
// this at the exe directory makes the real font loader work unmodified.
std::string AppState::getPluginDirectory() {
    char buffer[MAX_PATH] = {0};
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return "";
    }

    std::string path(buffer, length);
    size_t separator = path.find_last_of("\\/");
    return separator == std::string::npos ? "" : path.substr(0, separator);
}

std::string AppState::getXPlaneDirectory() {
    return "";
}

std::string AppState::getPreferencesFilePath() {
    return "";
}

std::string AppState::getLegacyPreferencesFilePath() {
    return "";
}

void AppState::migrateLegacyPreferences() {}

void AppState::executeAfter(int /*milliseconds*/, void * /*owner*/, std::function<void()> func) {
    func();
}

void AppState::executeAfterDebounced(std::string /*taskName*/, int /*milliseconds*/, void * /*owner*/, std::function<void()> func) {
    func();
}

void AppState::cancelTasksForOwner(void * /*owner*/) {}

std::string AppState::readPreference(const std::string & /*key*/, const std::string &defaultValue) {
    return defaultValue;
}

void AppState::writePreference(const std::string & /*key*/, const std::string & /*value*/) {}

void AppState::update() {}

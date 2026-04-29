// Minimal AppState implementation for the standalone Windows stress test.
// Replaces the full X-Plane-dependent appstate.cpp.
// - pluginInitialized is always true
// - executeAfter / executeAfterDebounced call the function immediately
//   (usbcontroller_win.cpp uses executeAfter(0, lambda) to create devices)
// - Everything else is a no-op / sensible default

#include "appstate.h"

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

std::string AppState::getPluginDirectory() {
    return "";
}

void AppState::executeAfter(int /*milliseconds*/, std::function<void()> func) {
    func();
}

void AppState::executeAfterDebounced(std::string /*taskName*/, int /*milliseconds*/, std::function<void()> func) {
    func();
}

std::string AppState::readPreference(const std::string & /*key*/, const std::string &defaultValue) {
    return defaultValue;
}

void AppState::writePreference(const std::string & /*key*/, const std::string & /*value*/) {}

void AppState::update() {}

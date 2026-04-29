cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(fmc-stresstest C CXX)

# Root of the winctrl plugin repository (two levels up from this folder)
set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(USBDEVICE_DIR "${ROOT_DIR}/src/include/utils/usbdevice")
set(INCLUDE_DIR   "${ROOT_DIR}/src/include")
set(STUBS_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/stubs")
set(FONTS_DIR     "${ROOT_DIR}/src/include/products/fmc/fonts")

add_executable(fmc-stresstest
    # ---------- stress test sources (this folder) ----------
    main.cpp
    stress_fmc.cpp

    # Cross-platform USBDevice methods + minimal device factory.
    usbdevice_shared.cpp

    # AppState stub
    stubs/appstate_stub.cpp

    # Shared utility
    ${ROOT_DIR}/src/include/utils/power-scheme.cpp

    # ---------- reused 1:1 from the main project ----------
    ${USBDEVICE_DIR}/usbdevice_win.cpp
    ${USBDEVICE_DIR}/usbcontroller_win.cpp
)

# IBM=1 enables the #if IBM blocks in usbdevice_win.cpp / usbcontroller_win.cpp.
# APL=0 / LIN=0 suppress the other platform guards.
target_compile_definitions(fmc-stresstest PRIVATE
    IBM=1
    APL=0
    LIN=0
)

target_include_directories(fmc-stresstest PRIVATE
    # Stubs directory must come first so our XPLMUtilities.h stub shadows the SDK one.
    ${STUBS_DIR}

    # Stress test own headers
    ${CMAKE_CURRENT_SOURCE_DIR}

    # Core plugin headers
    ${INCLUDE_DIR}               # appstate.h, config.h
    ${INCLUDE_DIR}/utils         # logger.hpp, power-scheme.h
    ${USBDEVICE_DIR}             # usbdevice.h, usbcontroller.h

    # FMC font data (default.h — self-contained, no X-Plane deps)
    ${FONTS_DIR}
)

# Windows HID libraries required by usbdevice_win.cpp / usbcontroller_win.cpp
target_link_libraries(fmc-stresstest PRIVATE hid setupapi powrprof)

# Statically link all MinGW-provided runtimes (libgcc, libstdc++, libpthread)
# so the exe needs no extra DLLs on the target Windows machine.
# Windows system DLLs (hid, setupapi, kernel32, etc.) remain dynamically linked.
target_link_options(fmc-stresstest PRIVATE -static)

# Helpful build message
message(STATUS "fmc-stresstest: reusing usbdevice_win.cpp and usbcontroller_win.cpp from ${USBDEVICE_DIR}")
message(STATUS "fmc-stresstest: cross-compile with  cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchain-win.cmake .")

// FMC Windows Stress Test — interactive menu edition

#include "diaglog.h"
#include "power-scheme.h"
#include "stress_fmc.h"
#include "usbcontroller.h"

#include <conio.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>

// ---------------------------------------------------------------------------
// Diagnostics log — see diaglog.h. Open for the whole session, so a device drop
// that happens outside a guided run is still recorded, together with everything
// the plugin's Logger emits.
// ---------------------------------------------------------------------------

// Writes to stdout and to the diagnostics log.
static void logLine(const char *format, ...) {
    char buffer[2048] = {0};

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    printf("%s", buffer);
    fflush(stdout);
    diagLogWrite(buffer);
}

// Asks a question, records the answer. Empty answer is logged as "(no answer)"
// so a skipped question is visible rather than silent.
static void askAndLog(const char *question) {
    printf("\n%s\n> ", question);
    fflush(stdout);

    char answer[512] = {0};
    if (!fgets(answer, sizeof(answer), stdin)) {
        answer[0] = '\0';
    }

    size_t length = strlen(answer);
    while (length > 0 && (answer[length - 1] == '\n' || answer[length - 1] == '\r')) {
        answer[--length] = '\0';
    }

    diagLogPrintf("Q: %s", question);
    diagLogPrintf("A: %s", length > 0 ? answer : "(no answer)");
}

// The controller's reaper deletes a device object as soon as the device stops
// enumerating, so a cached pointer goes dangling the moment the hardware drops.
// Everything below re-resolves the device instead of holding onto one.
static StressFMC *findFMC() {
    for (auto *dev : USBController::getInstance()->devices) {
        auto *fmc = dynamic_cast<StressFMC *>(dev);
        if (fmc) {
            return fmc;
        }
    }
    return nullptr;
}

static void waitForKey() {
    printf("\nPress Enter to exit...\n");
    fflush(stdout);
    (void) getchar();
}

static void clearScreen() {
    system("cls");
}

static void toggleHighPerformance() {
    clearScreen();
    if (!WindowsPowerScheme::isHighPerfEnabled()) {
        WindowsPowerScheme::enableHighPerformance();
        printf("High Performance power plan ENABLED.\n");
    } else {
        WindowsPowerScheme::restorePrevious();
        printf("Power plan RESTORED to previous setting.\n");
    }
    Sleep(1000);
}

static void printMenu(StressFMC *fmc) {
    clearScreen();
    printf("FMC Windows Stress Test\n");
    printf("=======================\n");
    printf("Device : %s (0x%04X)\n", fmc->productName.c_str(), fmc->productId);
    printf("Power  : %s\n\n", WindowsPowerScheme::isHighPerfEnabled() ? "[HIGH PERFORMANCE]" : "[default]");
    printf("  1. Start stress test (scrolling display + LED toggle)\n");
    printf("  2. Drain write queue\n");
    printf("  3. Toggle LEDs\n");
    printf("  4. Clear display\n");
    printf("  5. %s High Performance power plan\n",
        WindowsPowerScheme::isHighPerfEnabled() ? "Disable" : "Enable");
    printf("  6. Load font (raw file, no resize)\n");
    printf("  7. Load font through the plugin pipeline (pick MCDU or PFP geometry)\n");
    printf("  8. Draw glyph sheet (all printable ASCII)\n");
    printf("  9. Guided diagnostic run (logs to a file you can send on)\n");
    printf("  0. Dump every font x every geometry to dumps/ (no device writes)\n");
    printf("  Q. Quit\n\n");
    printf("> ");
    fflush(stdout);
}

// ---------------------------------------------------------------------------
// Menu option 1 — scrolling stress test.
// Press any key to stop and return to the menu.
// ---------------------------------------------------------------------------
static void runStressTest(StressFMC *fmc) {
    clearScreen();
    printf("Stress test running — press any key to stop.\n\n");
    fflush(stdout);

    int scrollOffset = 0;
    int frameCount = 0;
    bool ledsOn = false;

    while (!_kbhit()) {
        fmc->drawScrollingText(scrollOffset++);
        ++frameCount;

        if (frameCount % 10 == 0) {
            ledsOn = !ledsOn;
            fmc->setAllLedsEnabled(ledsOn);
            printf("\r[frame %6d] LEDs %s | write queue: %zu   ",
                frameCount, ledsOn ? "ON " : "OFF", fmc->getWriteQueueSize());
            fflush(stdout);
        }

        Sleep(50); // ~20 FPS
    }

    (void) _getch(); // consume the key
    fmc->setAllLedsEnabled(false);
    printf("\n\nStopped.\n");
    Sleep(600);
}

// ---------------------------------------------------------------------------
// Menu option 2 — wait until write queue drains to zero.
// ---------------------------------------------------------------------------
static void drainQueue(StressFMC *fmc) {
    clearScreen();
    printf("Draining write queue...\n\n");
    fflush(stdout);

    while (true) {
        size_t qs = fmc->getWriteQueueSize();
        printf("\rQueue size: %zu   ", qs);
        fflush(stdout);
        if (qs == 0) {
            break;
        }
        Sleep(50);
    }

    printf("\nQueue is empty.\n");
    Sleep(800);
}

// ---------------------------------------------------------------------------
// Menu option 3 — toggle all LEDs once.
// ---------------------------------------------------------------------------
static void toggleLeds(StressFMC *fmc) {
    static bool ledsOn = false;
    ledsOn = !ledsOn;
    fmc->setAllLedsEnabled(ledsOn);

    clearScreen();
    printf("LEDs toggled %s.\n", ledsOn ? "ON" : "OFF");
    Sleep(800);
}

// ---------------------------------------------------------------------------
// Menu option 4 — clear the FMC display.
// ---------------------------------------------------------------------------
static void clearFMCDisplay(StressFMC *fmc) {
    // Send 16 blank lines (mirrors ProductFMC::clearDisplay)
    std::vector<uint8_t> blankLine;
    blankLine.push_back(0xf2);
    for (int i = 0; i < StressFMC::PageCharsPerLine; ++i) {
        blankLine.push_back(0x42);
        blankLine.push_back(0x00);
        blankLine.push_back(' ');
    }
    for (int i = 0; i < 16; ++i) {
        fmc->writeData(blankLine);
    }

    clearScreen();
    printf("Display cleared.\n");
    Sleep(800);
}

// ---------------------------------------------------------------------------
// Menu option 6 — pick a font and upload it. The upload is timed until the
// write queue drains, which makes it a direct benchmark for HID write
// throughput (a font is a few hundred 64-byte packets).
// ---------------------------------------------------------------------------
static void loadFont(StressFMC *fmc) {
    clearScreen();
    printf("Load font\n");
    printf("=========\n\n");
    for (size_t i = 0; i < StressFMC::fontCount(); ++i) {
        printf("  %zu. %s\n", i + 1, StressFMC::fontName(i));
    }
    printf("  Any other key: cancel\n\n");
    printf("> ");
    fflush(stdout);

    int ch = _getch();
    size_t index = static_cast<size_t>(ch - '1');
    if (index >= StressFMC::fontCount()) {
        return;
    }

    printf("%c\n\nUploading %s font...\n", ch, StressFMC::fontName(index));
    fflush(stdout);

    DWORD start = GetTickCount();
    fmc->loadFont(index);
    while (fmc->getWriteQueueSize() > 0) {
        Sleep(1);
    }
    printf("Done: queue drained in %lu ms.\n", GetTickCount() - start);
    Sleep(1500);
}

// ---------------------------------------------------------------------------
// Menu option 7 — upload a font through the plugin's own pipeline
// (Font::GlyphData + Font::ResizeCellHeight + screen position), for either the
// MCDU geometry (23x29, no rebuild) or the PFP geometry (23x32, full rebuild).
// The packets stay addressed to the connected MCDU either way, so the PFP path
// can be exercised without PFP hardware.
// ---------------------------------------------------------------------------
static void loadFontThroughPipeline(StressFMC *fmc) {
    clearScreen();
    printf("Load font through the plugin pipeline\n");
    printf("=====================================\n\n");
    if (StressFMC::fontCount() == 0) {
        printf("No fonts found, so there is nothing to upload:\n\n%s\n", StressFMC::fontsReport().c_str());
        printf("Press any key to return to the menu.\n");
        fflush(stdout);
        (void) _getch();
        return;
    }
    for (size_t i = 0; i < StressFMC::fontCount(); ++i) {
        printf("  %zu. %s\n", i + 1, StressFMC::fontName(i));
    }
    printf("  Any other key: cancel\n\n");
    printf("> ");
    fflush(stdout);

    int ch = _getch();
    size_t index = static_cast<size_t>(ch - '1');
    if (index >= StressFMC::fontCount()) {
        return;
    }

    printf("%c\n\nGeometry:\n", ch);
    printf("  1. MCDU   (23 x 29, origin 16/17) - resize is a no-op\n");
    printf("  2. PFP 3N (23 x 32, origin 14/4)  - full ResizeCellHeight rebuild\n");
    printf("  Any other key: cancel\n\n");
    printf("> ");
    fflush(stdout);

    FMCHardwareType hardwareType;
    switch (_getch()) {
        case '1':
            hardwareType = FMCHardwareType::HARDWARE_MCDU;
            break;
        case '2':
            hardwareType = FMCHardwareType::HARDWARE_PFP3N;
            break;
        default:
            return;
    }

    printf("\n\nUploading %s...\n", StressFMC::fontName(index));
    fflush(stdout);

    DWORD start = GetTickCount();
    size_t packets = fmc->loadFontForHardware(index, hardwareType);
    while (fmc->getWriteQueueSize() > 0) {
        Sleep(1);
    }

    if (packets == 0) {
        printf("FAILED: nothing was sent.\n");
    } else {
        printf("Sent %zu packets, queue drained in %lu ms.\n", packets, GetTickCount() - start);
        printf("Drawing glyph sheet so missing glyphs are visible...\n");
        fmc->drawGlyphSheet();
        while (fmc->getWriteQueueSize() > 0) {
            Sleep(1);
        }
    }

    printf("\nPress any key to return to the menu.\n");
    fflush(stdout);
    (void) _getch();
}

// ---------------------------------------------------------------------------
// Menu option 8 — draw the glyph sheet on demand.
// ---------------------------------------------------------------------------
static void drawGlyphSheet(StressFMC *fmc) {
    clearScreen();
    printf("Drawing glyph sheet (all printable ASCII)...\n");
    fmc->drawGlyphSheet();
    while (fmc->getWriteQueueSize() > 0) {
        Sleep(1);
    }
    printf("Done. Blank cells are glyphs the device did not store.\n");
    Sleep(1500);
}

// ---------------------------------------------------------------------------
// Menu option 9 — guided diagnostic run. Performs each step of the font test
// in order, asks what the display shows, and writes both the technical output
// and the answers to stresstest-log.txt for sending on.
// ---------------------------------------------------------------------------
static void runDiagnostics(StressFMC *fmc) {
    clearScreen();

    printf("Guided diagnostic run\n");
    printf("=====================\n");
    printf("Writing to %s\n", diagLogPath());
    printf("Answer each question in plain words and press Enter. Just Enter skips.\n");
    printf("\nOne upload per run: replug the device between runs, then pick the next case.\n");
    printf("Press any key to start.\n");
    fflush(stdout);
    (void) _getch();

    SYSTEMTIME now;
    GetLocalTime(&now);
    logLine("\nWINCTRL FMC stress test - font diagnostics\n");
    logLine("=========================================\n");
    logLine("Started      : %04d-%02d-%02d %02d:%02d:%02d\n", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    logLine("Device       : %s (vendor 0x%04X, product 0x%04X)\n", fmc->productName.c_str(), fmc->vendorId, fmc->productId);
    logLine("Power plan   : %s\n", WindowsPowerScheme::isHighPerfEnabled() ? "high performance" : "default");
    logLine("Write queue  : %zu packets\n", fmc->getWriteQueueSize());
    logLine("\nFont discovery\n--------------\n%s", StressFMC::fontsReport().c_str());

    if (StressFMC::fontCount() == 0) {
        logLine("\nSTOPPED: no fonts available, so no upload can be tested.\n");
        logLine("Copy the repository's fonts/ directory next to the exe and run this again.\n");
        printf("\nStopped: no fonts found. The log explains where they were looked for.\n");
        printf("Press any key to return to the menu.\n");
        fflush(stdout);
        (void) _getch();
        return;
    }

    // Pick the font. winctrl.xpwwf IS the device's default font, so uploading it
    // proves nothing: pick a visibly different one (boeing-737, vga) or a change
    // cannot be seen at all.
    printf("\nWhich font?\n\n");
    for (size_t i = 0; i < StressFMC::fontCount(); ++i) {
        printf("  %zu. %s%s\n", i + 1, StressFMC::fontName(i), i == 0 ? "   <- same as the factory font, avoid" : "");
    }
    printf("  Any other key: cancel\n\n> ");
    fflush(stdout);

    size_t fontIndex = static_cast<size_t>(_getch() - '1');
    if (fontIndex >= StressFMC::fontCount()) {
        return;
    }
    logLine("\nFont under test: %s (%s)\n", StressFMC::fontName(fontIndex), StressFMC::fontFileName(fontIndex));

    // One upload per run, on purpose. The first diagnostic run showed that only
    // the first font upload after a USB connect takes effect: uploads 2 and 3 in
    // that session changed nothing, including the one that went back to MCDU
    // geometry. So every geometry has to be tested as upload #1 of a fresh
    // session, otherwise a failure cannot be told apart from "the device had
    // already committed a font set".
    printf("\nWhich case? The device must have been replugged just before this run.\n\n");
    printf("  1. MCDU geometry   (23 x 29, resize no-ops)\n");
    printf("  2. PFP 3N geometry (23 x 32, full rebuild)\n");
    printf("  3. No upload, just draw the glyph sheet on the font already there\n");
    printf("  4. PFP 3N geometry x4 back to back (the plugin's old behaviour)\n");
    printf("  Any other key: cancel\n\n> ");
    fflush(stdout);

    int choice = _getch();
    const char *caseName = nullptr;
    FMCHardwareType hardwareType = FMCHardwareType::HARDWARE_MCDU;
    switch (choice) {
        case '1':
            caseName = "MCDU geometry (23x29)";
            hardwareType = FMCHardwareType::HARDWARE_MCDU;
            break;
        case '2':
            caseName = "PFP 3N geometry (23x32)";
            hardwareType = FMCHardwareType::HARDWARE_PFP3N;
            break;
        case '3':
            caseName = "no upload, glyph sheet only";
            break;
        case '4':
            // The pre-dedupe plugin sent the font once per profile load, four times
            // per aircraft load. The device commits one font set per session, so
            // four overlapping upload sequences is the one field condition no test
            // has covered yet.
            caseName = "PFP 3N geometry, 4 uploads back to back";
            hardwareType = FMCHardwareType::HARDWARE_PFP3N;
            break;
        default:
            return;
    }

    clearScreen();
    logLine("\nCase         : %s\n", caseName);
    logLine("Uploads so far this session (connect() sends one): %d\n", fmc->uploadsSinceConnect);
    printf("Running: %s\n", caseName);
    fflush(stdout);

    askAndLog("Before the upload: what is on the screen right now? (factory font, previous font, blanks)");

    DWORD start = GetTickCount();
    if (choice != '3') {
        int uploads = choice == '4' ? 4 : 1;
        for (int i = 0; i < uploads; ++i) {
            size_t packets = fmc->loadFontForHardware(fontIndex, hardwareType);
            logLine("Pipeline     : %s\n", fmc->lastUploadReport.c_str());
            logLine("Packets sent : %zu\n", packets);
            if (packets == 0) {
                logLine("FAILED       : nothing was sent.\n");
            }
        }
    }

    fmc->drawGlyphSheet();
    while (fmc->getWriteQueueSize() > 0) {
        Sleep(1);
    }
    logLine("Drain time   : %lu ms\n", GetTickCount() - start);

    askAndLog("After the upload: did the font change? Full glyph sheet, blanks, garbled, or no change at all?");
    askAndLog("Anything else worth noting? (Enter to skip)");

    logLine("\nRun finished. Replug the device before testing another geometry.\n");

    clearScreen();
    printf("Done. Results appended to:\n  %s\n\n", diagLogPath());
    printf("To test another geometry: replug the device, restart this exe, run 9 again.\n\n");
    printf("Press any key to return to the menu.\n");
    fflush(stdout);
    (void) _getch();
}

// ---------------------------------------------------------------------------

int main() {
    clearScreen();
    printf("FMC Windows Stress Test\n");
    printf("=======================\n");
    printf("Scanning for MCDU devices (vendor 0x%04X)...\n", WINCTRL_VENDOR_ID);

    // Opened before the controller starts, so device creation and any early
    // failure is captured too.
    diagLogOpen();
    fflush(stdout);

    auto *controller = USBController::getInstance();

    printf("Found %zu device(s):\n", controller->devices.size());
    for (auto *dev : controller->devices) {
        printf("  0x%04X / 0x%04X  %s\n", dev->vendorId, dev->productId, dev->productName.c_str());
    }
    fflush(stdout);

    StressFMC *fmc = findFMC();
    if (!fmc) {
        printf("\nERROR: No MCDU device found. Make sure the device is connected.\n");
        diagLogPrintf("No MCDU device found at startup.");
        controller->destroy();
        waitForKey();
        return 1;
    }

    diagLogPrintf("Session start: %s (vendor 0x%04X, product 0x%04X)", fmc->productName.c_str(), fmc->vendorId, fmc->productId);

    // Menu loop. The device pointer is re-resolved every iteration: if the
    // hardware drops, the reaper deletes the object, and a cached pointer would
    // be used after free the next time a menu item ran.
    while (true) {
        fmc = findFMC();
        if (!fmc) {
            clearScreen();
            printf("Device disconnected. Waiting for it to come back...\n");
            printf("Press Q to quit, any other key to re-check.\n\n> ");
            fflush(stdout);
            diagLogPrintf("Device gone: no MCDU present, waiting for re-enumeration.");

            int key = _getch();
            if (key == 'q' || key == 'Q') {
                goto quit;
            }
            continue;
        }

        printMenu(fmc);

        int ch = _getch();

        switch (ch) {
            case '1':
                runStressTest(fmc);
                break;
            case '2':
                drainQueue(fmc);
                break;
            case '3':
                toggleLeds(fmc);
                break;
            case '4':
                clearFMCDisplay(fmc);
                break;
            case '5':
                toggleHighPerformance();
                break;
            case '6':
                loadFont(fmc);
                break;
            case '7':
                loadFontThroughPipeline(fmc);
                break;
            case '8':
                drawGlyphSheet(fmc);
                break;
            case '9':
                runDiagnostics(fmc);
                break;
            case '0':
                clearScreen();
                printf("Dumping every font x geometry to dumps/ ...\n\n");
                fflush(stdout);
                diagLogPrintf("--- dump run ---");
                logLine("Wrote %zu files.\n", StressFMC::dumpAllFontsAndGeometries());
                printf("\nPress any key to return to the menu.\n");
                fflush(stdout);
                (void) _getch();
                break;
            case 'q':
            case 'Q':
                goto quit;
            default:
                break;
        }
    }

quit:
    printf("\nShutting down...\n");
    WindowsPowerScheme::restorePrevious();
    if (StressFMC *present = findFMC()) {
        present->setAllLedsEnabled(false);
    }
    diagLogPrintf("Session end.");
    controller->destroy();
    diagLogClose();
    waitForKey();
    return 0;
}

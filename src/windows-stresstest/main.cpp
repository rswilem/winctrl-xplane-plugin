// FMC Windows Stress Test — interactive menu edition

#include "stress_fmc.h"
#include "power-scheme.h"
#include "usbcontroller.h"

#include <conio.h>
#include <cstdio>
#include <windows.h>

static void waitForKey() {
    printf("\nPress Enter to exit...\n");
    fflush(stdout);
    (void)getchar();
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

    int  scrollOffset = 0;
    int  frameCount   = 0;
    bool ledsOn       = false;

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

    (void)_getch(); // consume the key
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
        if (qs == 0) break;
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

int main() {
    clearScreen();
    printf("FMC Windows Stress Test\n");
    printf("=======================\n");
    printf("Scanning for MCDU devices (vendor 0x%04X)...\n", WINCTRL_VENDOR_ID);
    fflush(stdout);

    auto *controller = USBController::getInstance();

    printf("Found %zu device(s):\n", controller->devices.size());
    for (auto *dev : controller->devices) {
        printf("  0x%04X / 0x%04X  %s\n", dev->vendorId, dev->productId, dev->productName.c_str());
    }
    fflush(stdout);

    StressFMC *fmc = nullptr;
    for (auto *dev : controller->devices) {
        fmc = dynamic_cast<StressFMC *>(dev);
        if (fmc) break;
    }

    if (!fmc) {
        printf("\nERROR: No MCDU device found. Make sure the device is connected.\n");
        controller->destroy();
        waitForKey();
        return 1;
    }

    // Menu loop
    while (true) {
        printMenu(fmc);

        int ch = _getch();

        switch (ch) {
            case '1': runStressTest(fmc);   break;
            case '2': drainQueue(fmc);      break;
            case '3': toggleLeds(fmc);      break;
            case '4': clearFMCDisplay(fmc);      break;
            case '5': toggleHighPerformance();    break;
            case 'q': case 'Q': goto quit;
            default: break;
        }
    }

quit:
    printf("\nShutting down...\n");
    WindowsPowerScheme::restorePrevious();
    fmc->setAllLedsEnabled(false);
    controller->destroy();
    waitForKey();
    return 0;
}


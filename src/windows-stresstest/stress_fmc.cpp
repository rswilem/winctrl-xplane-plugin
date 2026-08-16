// StressFMC implementation.
//
// Init packets are copied verbatim from ProductFMC::connect() in product-fmc.cpp.
// The identifierByte is hardcoded to 0x32 (MCDU), matching the sniffed source.
//
// Fonts are read from the .xpwwf files in the plugin's fonts/ directory, the
// same files the plugin ships. There are two upload paths:
//   loadFont()            - the raw file, packet for packet. For MCDU the
//                           hardware conversion is a no-op (the sniffed data
//                           already uses 0x32), so no font.cpp is involved.
//   loadFontForHardware() - the plugin's real pipeline (Font::GlyphData +
//                           Font::ResizeCellHeight), for any hardware geometry.
//                           See TESTING.md for what this is for.
//
// The draw path mirrors ProductFMC::draw():
//   For each character: push {0x42, 0x00, char} into a flat buffer, then send
//   it in 63-byte chunks each prefixed with 0xf2 via writeData() (usbdevice_win.cpp).

#include "stress_fmc.h"

#include "font.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <windows.h>

// ---------------------------------------------------------------------------
// Selectable fonts, discovered from the .xpwwf files in a fonts/ directory.
// Index 0 is the font uploaded by connect().
// ---------------------------------------------------------------------------
namespace {
    struct StressFont {
            std::string name;
            std::filesystem::path path;
    };

    // The MCDU font the plugin treats as its default; hoisted to index 0 so
    // connect() uploads the same font it always has.
    const char *kDefaultFontFile = "winctrl.xpwwf";

    // Excluded: it targets different hardware (100-byte packets, not the MCDU
    // format this harness hardcodes via identifierByte 0x32).
    const char *kExcludedFontFile = "md11-cdu.xpwwf";

    std::filesystem::path executableDirectory() {
        char buffer[MAX_PATH] = {0};
        DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) {
            return {};
        }
        return std::filesystem::path(buffer).parent_path();
    }

    // fonts/ next to the exe wins, so the harness can be copied to a test
    // machine with its fonts alongside. STRESS_FONTS_DIR is the repository's
    // fonts/ directory, baked in by CMake for running straight from a build dir.
    std::filesystem::path fontsDirectory() {
        std::error_code ec;

        std::filesystem::path local = executableDirectory() / "fonts";
        if (std::filesystem::is_directory(local, ec)) {
            return local;
        }

#ifdef STRESS_FONTS_DIR
        std::filesystem::path repo(STRESS_FONTS_DIR);
        if (std::filesystem::is_directory(repo, ec)) {
            return repo;
        }
#endif

        return {};
    }

    const std::vector<StressFont> &fonts() {
        static const std::vector<StressFont> discovered = [] {
            std::vector<StressFont> result;

            std::filesystem::path directory = fontsDirectory();
            if (directory.empty()) {
                printf("[StressFMC] No fonts directory found; font upload is unavailable.\n");
                return result;
            }

            std::error_code ec;
            for (const auto &entry : std::filesystem::directory_iterator(directory, ec)) {
                if (!entry.is_regular_file() || entry.path().extension() != ".xpwwf") {
                    continue;
                }
                if (entry.path().filename() == kExcludedFontFile) {
                    continue;
                }
                result.push_back({entry.path().stem().string(), entry.path()});
            }

            std::sort(result.begin(), result.end(), [](const StressFont &a, const StressFont &b) {
                if ((a.path.filename() == kDefaultFontFile) != (b.path.filename() == kDefaultFontFile)) {
                    return a.path.filename() == kDefaultFontFile;
                }
                return a.name < b.name;
            });

            printf("[StressFMC] Found %zu fonts in %s\n", result.size(), directory.string().c_str());
            return result;
        }();

        return discovered;
    }

    // Same length-prefixed format the plugin reads: <len:u8><len bytes> repeated.
    std::vector<std::vector<unsigned char>> readFontFile(const std::filesystem::path &path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            printf("[StressFMC] Could not open font file: %s\n", path.string().c_str());
            return {};
        }

        std::vector<std::vector<unsigned char>> packets;
        unsigned char lengthByte = 0;

        while (file.read(reinterpret_cast<char *>(&lengthByte), sizeof(lengthByte))) {
            if (lengthByte == 0) {
                break;
            }

            std::vector<unsigned char> packet(lengthByte, 0);
            if (!file.read(reinterpret_cast<char *>(packet.data()), lengthByte)) {
                printf("[StressFMC] Truncated font file: %s\n", path.string().c_str());
                break;
            }
            packets.push_back(std::move(packet));
        }

        return packets;
    }
} // namespace

size_t StressFMC::fontCount() {
    return fonts().size();
}

const char *StressFMC::fontName(size_t index) {
    return index < fontCount() ? fonts()[index].name.c_str() : "?";
}

std::string StressFMC::fontsReport() {
    std::error_code ec;
    std::string report;

    std::filesystem::path local = executableDirectory() / "fonts";
    report += "  exe directory      : " + executableDirectory().string() + "\n";
    report += "  fonts next to exe  : " + local.string() + (std::filesystem::is_directory(local, ec) ? "  [EXISTS]\n" : "  [MISSING]\n");

#ifdef STRESS_FONTS_DIR
    std::filesystem::path repo(STRESS_FONTS_DIR);
    report += "  build-time fonts   : " + repo.string() + (std::filesystem::is_directory(repo, ec) ? "  [EXISTS]\n" : "  [MISSING]\n");
#else
    report += "  build-time fonts   : (not compiled in)\n";
#endif

    std::filesystem::path resolved = fontsDirectory();
    report += "  resolved directory : " + (resolved.empty() ? std::string("(none)") : resolved.string()) + "\n";
    report += "  fonts found        : " + std::to_string(fontCount()) + "\n";

    if (fontCount() == 0) {
        report += "\n  No fonts. Copy the repository's fonts/ directory next to the exe.\n";
        return report;
    }

    for (size_t i = 0; i < fontCount(); ++i) {
        std::vector<std::vector<unsigned char>> packets = readFontFile(fonts()[i].path);
        size_t bytes = 0;
        for (const auto &packet : packets) {
            bytes += packet.size();
        }
        report += "    " + std::to_string(i + 1) + ". " + fonts()[i].path.filename().string() + " - " + std::to_string(packets.size()) + " packets, " + std::to_string(bytes) + " bytes, first packet " + (packets.empty() ? "n/a" : std::to_string(packets[0].size())) + " bytes\n";
    }

    return report;
}

const char *StressFMC::fontFileName(size_t index) {
    static std::string filename;
    filename = index < fontCount() ? fonts()[index].path.filename().string() : "";
    return filename.c_str();
}

void StressFMC::loadFont(size_t index) {
    if (index >= fontCount()) {
        return;
    }
    ++uploadsSinceConnect;
    for (const auto &packet : readFontFile(fonts()[index].path)) {
        writeData(std::vector<uint8_t>(packet.begin(), packet.end()));
    }
}

// ---------------------------------------------------------------------------
// The plugin's own font pipeline, for the geometry of any hardware type.
// Mirrors ProductFMC::setFont() step for step so a failure here is a failure
// in font.cpp, not in this harness.
// ---------------------------------------------------------------------------
size_t StressFMC::loadFontForHardware(size_t index, FMCHardwareType hardwareType) {
    lastUploadReport = {};

    if (index >= fontCount()) {
        lastUploadReport = "no font at index " + std::to_string(index);
        return 0;
    }

    // identifierByte stays 0x32 so the packets remain addressed to the MCDU that
    // is actually plugged in; hardwareType selects the grid origin patch.
    std::vector<std::vector<unsigned char>> font = Font::GlyphData(fontFileName(index), identifierByte, hardwareType);
    if (font.empty()) {
        lastUploadReport = "Font::GlyphData returned nothing for " + std::string(fontName(index));
        printf("[StressFMC] %s\n", lastUploadReport.c_str());
        return 0;
    }

    FMCScreenLayout layout = FMCHardwareMapping::ScreenLayoutForHardware(hardwareType);

    // ResizeCellHeight returns true both when it rebuilds and when it no-ops at or
    // below the authored 29px height, so the return value alone cannot tell them
    // apart. The packet count can: a rebuild changes it, a no-op does not.
    size_t packetsBefore = font.size();
    bool parsed = Font::ResizeCellHeight(font, layout.characterHeight, layout.characterWidth);
    const char *outcome = !parsed ? "PARSE FAILED, font sent unchanged"
        : font.size() != packetsBefore  ? "rebuilt"
                                        : "no-op (height <= 29), font sent as authored";

    ++uploadsSinceConnect;
    lastUploadReport = "cell " + std::to_string(static_cast<int>(layout.characterWidth)) + "x" + std::to_string(static_cast<int>(layout.characterHeight)) + ", origin " + std::to_string(static_cast<int>(layout.x)) + "/" + std::to_string(static_cast<int>(layout.y)) + ", resize " + outcome + ", " + std::to_string(packetsBefore) + " -> " + std::to_string(font.size()) + " packets, upload #" + std::to_string(uploadsSinceConnect) + " since connect";
    printf("[StressFMC] %s\n", lastUploadReport.c_str());

    for (const auto &packet : font) {
        writeData(std::vector<uint8_t>(packet.begin(), packet.end()));
    }

    showBackgroundBlack();
    setScreenPosition(layout.x, layout.y);

    return font.size();
}

// ---------------------------------------------------------------------------
// Offline capture: every font, every hardware geometry, straight to disk.
// No device writes, so it needs no replug and consumes no font-set commit.
// ---------------------------------------------------------------------------
size_t StressFMC::dumpAllFontsAndGeometries() {
    const struct {
            const char *name;
            FMCHardwareType type;
    } hardwareTypes[] = {
        {"mcdu", FMCHardwareType::HARDWARE_MCDU},
        {"pfp3n", FMCHardwareType::HARDWARE_PFP3N},
        {"pfp4", FMCHardwareType::HARDWARE_PFP4},
        {"pfp7", FMCHardwareType::HARDWARE_PFP7},
    };

    std::error_code ec;
    std::filesystem::path outputDirectory = executableDirectory() / "dumps";
    std::filesystem::create_directories(outputDirectory, ec);

    size_t written = 0;
    for (size_t i = 0; i < fontCount(); ++i) {
        std::string file = fonts()[i].path.filename().string();

        for (const auto &hardware : hardwareTypes) {
            // Both device identifiers per geometry: 0x32 is what our MCDU accepts,
            // 0x35 is what a real PFP 3N is addressed as. The field failure only
            // ever happens on the second, which no MCDU test can cover.
            for (unsigned char identifier : {0x32, 0x35}) {
                std::vector<std::vector<unsigned char>> font = Font::GlyphData(file, identifier, hardware.type);
                if (font.empty()) {
                    Logger::getInstance()->critical("dump: GlyphData failed for %s\n", file.c_str());
                    continue;
                }

                FMCScreenLayout layout = FMCHardwareMapping::ScreenLayoutForHardware(hardware.type);
                size_t before = font.size();
                bool parsed = Font::ResizeCellHeight(font, layout.characterHeight, layout.characterWidth);

                char name[256] = {0};
                snprintf(name, sizeof(name), "%s-%s-id%02x-%dx%d.bin", fonts()[i].name.c_str(), hardware.name, identifier,
                    static_cast<int>(layout.characterWidth), static_cast<int>(layout.characterHeight));

                std::ofstream out(outputDirectory / name, std::ios::binary);
                size_t bytes = 0;
                for (const auto &packet : font) {
                    // Same length-prefixed framing as the .xpwwf files, so the dump
                    // can be parsed with the existing reader.
                    unsigned char length = static_cast<unsigned char>(packet.size());
                    out.write(reinterpret_cast<const char *>(&length), 1);
                    out.write(reinterpret_cast<const char *>(packet.data()), packet.size());
                    bytes += packet.size();
                }

                Logger::getInstance()->info("dump: %s - %zu -> %zu packets, %zu bytes, resize %s\n",
                    name, before, font.size(), bytes, !parsed ? "PARSE FAILED" : (font.size() != before ? "rebuilt" : "no-op"));
                ++written;
            }
        }
    }

    Logger::getInstance()->info("dump: wrote %zu files to %s\n", written, outputDirectory.string().c_str());
    return written;
}

// ---------------------------------------------------------------------------
// Screen position — copy of ProductFMC::setScreenPosition().
// ---------------------------------------------------------------------------
void StressFMC::setScreenPosition(unsigned char x, unsigned char y) {
    std::vector<uint8_t> packet(64, 0);
    packet[0] = 0xf0;
    packet[1] = 0x00;
    packet[2] = 0x00;
    packet[3] = 0x2a;

    uint8_t *p = packet.data() + 4;
    p[0] = identifierByte;
    p[1] = 0xbb;
    p[2] = 0x00;
    p[3] = 0x00;
    p[4] = 0x18;
    p[5] = 0x01;
    p[6] = 0x00;
    p[7] = 0x00;
    p[8] = 0x00;
    p[9] = 0x00;
    p[10] = 0x00;
    p[11] = 0x00;
    p[12] = 0x00;
    p[13] = 0x08;
    p[14] = 0x00;
    p[15] = 0x00;
    p[16] = 0x00;
    p[17] = static_cast<uint8_t>(36 + x);
    p[18] = 0x00;
    p[19] = static_cast<uint8_t>(20 + y);
    p[20] = 0x00;
    p[21] = 0x0e;
    p[22] = 0x00;
    p[23] = 0x18;
    p[24] = 0x00;

    uint8_t *c = packet.data() + 29;
    c[0] = identifierByte;
    c[1] = 0xbb;
    c[2] = 0x00;
    c[3] = 0x00;
    c[4] = 0x05;
    c[5] = 0x01;
    c[6] = 0x00;
    c[7] = 0x00;
    c[8] = 0x00;
    c[9] = 0x00;
    c[10] = 0x00;
    c[11] = 0x00;
    c[12] = 0x01;
    c[13] = 0x00;
    c[14] = 0x00;
    c[15] = 0x00;
    c[16] = 0x00;

    writeData(packet);
}

// ---------------------------------------------------------------------------
// Scrolling text source — uppercase ASCII, wraps around.
// ---------------------------------------------------------------------------
static const char kLoremIpsum[] =
    "LOREM IPSUM DOLOR SIT AMET CONSECTETUR ADIPISCING ELIT SED DO EIUSMOD TEMPOR "
    "INCIDIDUNT UT LABORE ET DOLORE MAGNA ALIQUA UT ENIM AD MINIM VENIAM QUIS NOSTRUD "
    "EXERCITATION ULLAMCO LABORIS NISI UT ALIQUIP EX EA COMMODO CONSEQUAT DUIS AUTE "
    "IRURE DOLOR IN REPREHENDERIT IN VOLUPTATE VELIT ESSE CILLUM DOLORE EU FUGIAT NULLA "
    "PARIATUR EXCEPTEUR SINT OCCAECAT CUPIDATAT NON PROIDENT SUNT IN CULPA QUI OFFICIA "
    "DESERUNT MOLLIT ANIM ID EST LABORUM  ";

static constexpr int kLoremLen = sizeof(kLoremIpsum) - 1; // exclude null terminator

// ---------------------------------------------------------------------------

StressFMC::StressFMC(HIDDeviceHandle hidDevice, uint16_t vendorId, uint16_t productId, std::string vendorName, std::string productName) : USBDevice(hidDevice, vendorId, productId, vendorName, productName) {
    connect();
}

StressFMC::~StressFMC() {
    setAllLedsEnabled(false);
    setLedBrightness(STRESS_LED_BACKLIGHT, 0);
    setLedBrightness(STRESS_LED_SCREEN_BACKLIGHT, 0);
}

const char *StressFMC::classIdentifier() {
    return "StressFMC (MCDU)";
}

bool StressFMC::connect() {
    if (!USBDevice::connect()) {
        return false;
    }

    uploadsSinceConnect = 0;

    sendInitPackets();

    // Deliberately no font upload here. The device accepts only the first
    // font-set commit per USB session, so uploading one on connect would make
    // every test upload the second one, which is exactly the confound that
    // invalidated the first diagnostic run. The device keeps its factory font
    // until a font is uploaded from the menu.

    setLedBrightness(STRESS_LED_BACKLIGHT, 128);
    setLedBrightness(STRESS_LED_SCREEN_BACKLIGHT, 128);
    setLedBrightness(STRESS_LED_OVERALL_BRIGHTNESS, 255);
    setAllLedsEnabled(false);
    showBackground_WINCTRL_LOGO();
    setLedBrightness(STRESS_LED_MCDU_START, 1); // MCDU_FAIL indicator

    printf("[StressFMC] Connected to %s (factory font, no upload yet)\n", productName.c_str());
    return true;
}

// ---------------------------------------------------------------------------
// Init packets — exact copy of the 17 writeData() calls in ProductFMC::connect()
// (product-fmc.cpp). identifierByte = 0x32 is hardcoded in the sniffed data.
// ---------------------------------------------------------------------------
void StressFMC::sendInitPackets() {
    writeData({0xf0, 0x0, 0x1, 0x38, 0x32, 0xbb, 0x0, 0x0, 0x1e, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x18, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x34, 0x0, 0x18, 0x0, 0xe, 0x0, 0x18, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x2, 0x38, 0x0, 0x0, 0x0, 0x1, 0x0, 0x5, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0xc4, 0x24, 0xa, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x1, 0x0, 0x6, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x3, 0x38, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0xff, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0xa5, 0xff, 0xff, 0x5, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x4, 0x38, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0xff, 0xff, 0xff, 0x6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0xff, 0x0, 0xff, 0x7, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x5, 0x38, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x3d, 0xff, 0x0, 0xff, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0xff, 0x63, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x6, 0x38, 0xff, 0xff, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0xff, 0xff, 0xa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x7, 0x38, 0x0, 0x0, 0x2, 0x0, 0x0, 0xff, 0xff, 0xff, 0xb, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x42, 0x5c, 0x61, 0xff, 0xc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x8, 0x38, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x77, 0x77, 0x77, 0xff, 0xd, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x2, 0x0, 0x5e, 0x73, 0x79, 0xff, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x9, 0x38, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x00, 0x00, 0x00, 0xff, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0xa5, 0xff, 0xff, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xa, 0x38, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0xff, 0xff, 0xff, 0xff, 0x11, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xb, 0x38, 0xff, 0x12, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x3d, 0xff, 0x0, 0xff, 0x13, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xc, 0x38, 0x0, 0x3, 0x0, 0xff, 0x63, 0xff, 0xff, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0x0, 0xff, 0xff, 0x15, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xd, 0x38, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x0, 0xff, 0xff, 0xff, 0x16, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x42, 0x5c, 0x61, 0xff, 0x17, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xe, 0x38, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x77, 0x77, 0x77, 0xff, 0x18, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x3, 0x0, 0x5e, 0x73, 0x79, 0xff, 0x19, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0xf, 0x38, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x10, 0x38, 0x1b, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x19, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0xe, 0x0, 0x0, 0x0, 0x4, 0x0, 0x2, 0x0, 0x0, 0x0, 0x1c, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x32, 0xbb, 0x0, 0x0, 0x1a, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
    writeData({0xf0, 0x0, 0x11, 0x12, 0x2, 0x32, 0xbb, 0x0, 0x0, 0x1c, 0x1, 0x0, 0x0, 0x76, 0x72, 0x19, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0});
}

// ---------------------------------------------------------------------------
// Background — WINCTRL_LOGO (variant value 8, so 0x0c + 8 = 0x14 in extra bytes)
// ---------------------------------------------------------------------------
void StressFMC::showBackground_WINCTRL_LOGO() {
    std::vector<uint8_t> data = {
        0xf0, 0x00, 0x0a, 0x12, identifierByte, 0xbb, 0x00, 0x00,
        0x04, 0x01, 0x00, 0x00, 0xd4, 0xac, 0x09, 0x00,
        // extra (48 bytes): [0x00, 0x01, 0x00, 0x00, 0x00, variant_byte, 0x00 * 42]
        0x00, 0x01, 0x00, 0x00, 0x00, 0x14,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00};
    writeData(data);
}

// Background — BLACK, the variant setFont() applies after every font upload.
void StressFMC::showBackgroundBlack() {
    writeData({0xf0, 0x00, 0x03, 0x12, identifierByte, 0xbb, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0xfd, 0x24, 0x07, 0x00});
}

// ---------------------------------------------------------------------------
// Glyph sheet — every printable ASCII code point, in order, one screenful.
// A glyph the device did not store renders blank, which is what the missing
// characters in the field reports look like.
// ---------------------------------------------------------------------------
void StressFMC::drawGlyphSheet() {
    std::vector<uint8_t> buf;
    buf.reserve(PageLines * PageCharsPerLine * PageBytesPerChar);

    unsigned int cell = 0;
    for (unsigned int row = 0; row < PageLines; ++row) {
        for (unsigned int col = 0; col < PageCharsPerLine; ++col, ++cell) {
            // 0x20..0x7e is 95 code points; wrap so the whole screen is filled.
            uint8_t character = static_cast<uint8_t>(0x20 + (cell % 95));
            buf.push_back(0x42); // white
            buf.push_back(0x00); // large
            buf.push_back(character);
        }
    }

    while (!buf.empty()) {
        size_t maxLength = std::min<size_t>(63, buf.size());
        std::vector<uint8_t> usbBuf(buf.begin(), buf.begin() + maxLength);
        usbBuf.insert(usbBuf.begin(), 0xf2);
        if (maxLength < 63) {
            usbBuf.insert(usbBuf.end(), 63 - maxLength, 0);
        }
        writeData(usbBuf);
        buf.erase(buf.begin(), buf.begin() + maxLength);
    }
}

// ---------------------------------------------------------------------------
// LED control — mirrors ProductFMC::setLedBrightness() / setAllLedsEnabled()
// ---------------------------------------------------------------------------
void StressFMC::setLedBrightness(uint8_t led, uint8_t brightness) {
    writeData({0x02, identifierByte, 0xbb, 0x00, 0x00, 0x03, 0x49, led, brightness, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void StressFMC::setAllLedsEnabled(bool enable) {
    for (uint8_t i = STRESS_LED_MCDU_START; i <= STRESS_LED_MCDU_END; ++i) {
        setLedBrightness(i, enable ? 1 : 0);
    }
}

// ---------------------------------------------------------------------------
// Display update — mirrors ProductFMC::draw() with white color (0x42, 0x00)
// and plain ASCII character passthrough (no special glyph mapping needed for
// uppercase A-Z / space, which is all lorem ipsum uses).
//
// Each row starts at a different offset in kLoremIpsum so every row is
// different every frame, exercising a full-screen repaint every call.
// ---------------------------------------------------------------------------
void StressFMC::drawScrollingText(int scrollOffset) {
    std::vector<uint8_t> buf;
    buf.reserve(PageLines * PageCharsPerLine * 3);

    for (unsigned int row = 0; row < PageLines; ++row) {
        for (unsigned int col = 0; col < PageCharsPerLine; ++col) {
            // Color + font bytes: white, normal size (matches COLOR_WHITE = 0x0042)
            buf.push_back(0x42);
            buf.push_back(0x00);
            // Character: each row starts at a unique position in the text
            int idx = (scrollOffset + static_cast<int>(row) + static_cast<int>(col)) % kLoremLen;
            buf.push_back(static_cast<uint8_t>(kLoremIpsum[idx]));
        }
    }

    // Send buffer in 63-byte chunks, each prefixed with 0xf2 (mirrors draw() in product-fmc.cpp)
    while (!buf.empty()) {
        size_t maxLength = std::min<size_t>(63, buf.size());
        std::vector<uint8_t> usbBuf(buf.begin(), buf.begin() + maxLength);
        usbBuf.insert(usbBuf.begin(), 0xf2);
        if (maxLength < 63) {
            usbBuf.insert(usbBuf.end(), 63 - maxLength, 0);
        }
        writeData(usbBuf);
        buf.erase(buf.begin(), buf.begin() + maxLength);
    }
}

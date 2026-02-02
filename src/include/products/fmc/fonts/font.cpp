#include "font.h"

#include "737.h"
#include "airbus.h"
#include "appstate.h"
#include "config.h"
#include "default.h"
#include "md11-cdu.h"
#include "vga_1.h"
#include "xcrafts.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <XPLMUtilities.h>

const std::vector<std::vector<unsigned char>> Font::GlyphData(std::string filename, unsigned char hardwareIdentifier, FMCHardwareType hardwareType) {
    std::string pluginDirectory = AppState::getInstance()->getPluginDirectory();
    if (pluginDirectory.empty()) {
        return {};
    }

    std::filesystem::path fontFile = std::filesystem::path(pluginDirectory) / "fonts" / filename;
    std::ifstream file(fontFile, std::ios::binary);
    if (!file) {
        debug_force("Could not open custom font file: %s\n", fontFile.c_str());
        return {};
    }

    std::vector<std::vector<unsigned char>> result = {};
    unsigned char lengthByte = 0;

    while (file.read(reinterpret_cast<char *>(&lengthByte), sizeof(lengthByte))) {
        if (lengthByte == 0) {
            break;
        }

        std::vector<unsigned char> glyphData(lengthByte, 0);
        if (file.read(reinterpret_cast<char *>(glyphData.data()), lengthByte)) {
            result.push_back(glyphData);
        } else {
            debug_force("Failed to read glyph data from file: %s\n", fontFile.c_str());
            break;
        }
    }

    convertGlyphDataForHardware(result, hardwareIdentifier, hardwareType);

    return result;
}

const std::vector<std::vector<unsigned char>> Font::GlyphData(FontVariant variant, unsigned char hardwareIdentifier, FMCHardwareType hardwareType) {
    std::vector<std::vector<unsigned char>> result = {};

    switch (variant) {
        case FontVariant::FontAirbus:
            result = fmcFontAirbus;
            break;

        case FontVariant::Font737:
            result = fmcFont737;
            break;

        case FontVariant::FontXCrafts:
            result = fmcFontXCrafts;
            break;

        case FontVariant::FontVGA1:
            result = fmcFontVGA1;
            break;

        case FontVariant::FontMD11:
            result = fmcFontMd11Cdu;
            break;

        case FontVariant::Default:
        default:
            result = fmcFontDefault;
            break;
    }

    convertGlyphDataForHardware(result, hardwareIdentifier, hardwareType);

    return result;
}

const std::vector<std::string> Font::ReadCustomFontFiles() {
    std::vector<std::string> fontFiles;

    std::string pluginDirectory = AppState::getInstance()->getPluginDirectory();
    if (pluginDirectory.empty()) {
        return fontFiles;
    }

    std::filesystem::path fontsDirectory = std::filesystem::path(pluginDirectory) / "fonts";

    try {
        if (std::filesystem::exists(fontsDirectory)) {
            for (const auto &entry : std::filesystem::directory_iterator(fontsDirectory)) {
                if (entry.is_regular_file() && entry.path().extension() == ".xpwwf") {
                    fontFiles.push_back(entry.path().filename().string());
                }
            }
        } else {
            debug_force("Fonts directory does not exist: %s\n", fontsDirectory.c_str());
        }
    } catch (const std::filesystem::filesystem_error &e) {
        debug_force("Error reading fonts directory: %s\n", e.what());
    }

    return fontFiles;
}

const bool Font::IsCustomFontAvailable(std::string filename) {
    std::vector<std::string> availableFonts = ReadCustomFontFiles();
    for (const auto &availableFont : availableFonts) {
        if (availableFont == filename) {
            return true;
        }
    }
    return false;
}

void Font::convertGlyphDataForHardware(std::vector<std::vector<unsigned char>> &data, unsigned char hardwareIdentifier, FMCHardwareType hardwareType) {
    for (auto &row : data) {
        for (size_t i = 0; i + 1 < row.size(); i++) {
            if (row[i] == 0x32 && row[i + 1] == 0xbb) { // Sniffed packets always have the MCDU identifier
                row[i] = hardwareIdentifier;
                row[i + 1] = 0xbb;
            }
        }

        if (row.size() > 23 && row[0] == 0xF0 && row[1] == 0x00 && row[17] == 0x08 && row[18] == 0x00 && row[21] == 0x34 && row[23] == 0x25) {
            uint8_t leftOffset = 16;
            uint8_t topOffset = 17;

            if (hardwareType == FMCHardwareType::HARDWARE_MCDU) {
                // MCDU default offsets Left 0x34, Top 0x25 (SAP L16, T17)
                leftOffset = 16;
                topOffset = 17;
            } else {
                // PFP default offsets Left 0x32, Top 0x20 (SAP L14, T12)
                leftOffset = 14;
                topOffset = 12;
            }

            row[21] = 36 + leftOffset;
            row[23] = 20 + topOffset;
        }
    }
}

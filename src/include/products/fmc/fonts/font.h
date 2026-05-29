#ifndef FONT_H
#define FONT_H

#include "fmc-hardware-mapping.h"

#include <vector>
enum class FontVariant : unsigned char {
    Default,
    FontAirbus,
    Font737,
    Font747,
    FontXCrafts,
    FontVGA1,
    FontMD11,
};

class Font {
    private:
        static void convertGlyphDataForHardware(std::vector<std::vector<unsigned char>> &data, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);

    public:
        static const std::vector<std::vector<unsigned char>> GlyphData(std::string filename, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);
        static const std::vector<std::vector<unsigned char>> GlyphData(FontVariant variant, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);
        static const std::vector<std::string> ReadCustomFontFiles();
        static const bool IsCustomFontAvailable(std::string filename);
};

#endif

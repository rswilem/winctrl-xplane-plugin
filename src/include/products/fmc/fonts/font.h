#ifndef FONT_H
#define FONT_H

#include "fmc-hardware-mapping.h"

#include <cstdint>
#include <string>
#include <vector>
enum class FontVariant : unsigned char {
    Default,
    FontAirbus,
    Font737,
    Font744,
    FontXCrafts,
    FontVGA1,
    FontMD11,
    FontLTN,
    FontQ4XP,
};

class Font {
    private:
        static void convertGlyphDataForHardware(std::vector<std::vector<unsigned char>> &data, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);

    public:
        // Every font ships as an .xpwwf file in the plugin's fonts/ directory;
        // nothing is compiled in. These two map a variant a profile asked for
        // onto its file, and a file onto the label the Plugins menu shows.
        static const std::string FilenameForVariant(FontVariant variant);
        static const std::string DisplayNameForFile(const std::string &filename);

        static const std::vector<std::vector<unsigned char>> GlyphData(std::string filename, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);
        static const std::vector<std::vector<unsigned char>> GlyphData(FontVariant variant, unsigned char hardwareIdentifier, FMCHardwareType hardwareType);
        static const std::vector<std::string> ReadCustomFontFiles();
        static const bool IsCustomFontAvailable(std::string filename);

        // Pads every glyph cell with blank rows (below the ink) up to cellHeight,
        // keeping each glyph's original ink. Characters keep their native size; the
        // extra height becomes row pitch. cellWidth patches the geometry width field
        // (default 23, the MCDU authored value). Returns false (and changes nothing)
        // if the font structure could not be parsed.
        static bool ResizeCellHeight(std::vector<std::vector<unsigned char>> &data, unsigned char cellHeight, unsigned char cellWidth = 23);

        static bool RemoveCommandBlocks(std::vector<std::vector<unsigned char>> &data, uint16_t functionId);};

#endif

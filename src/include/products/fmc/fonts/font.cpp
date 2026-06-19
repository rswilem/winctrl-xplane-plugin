#include "font.h"

#include "737.h"
#include "744.h"
#include "airbus.h"
#include "appstate.h"
#include "config.h"
#include "default.h"
#include "md11-cdu.h"
#include "vga_1.h"
#include "xcrafts.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
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
        Logger::getInstance()->error("Could not open custom font file: %s\n", fontFile.c_str());
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
            Logger::getInstance()->error("Failed to read glyph data from file: %s\n", fontFile.c_str());
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

        case FontVariant::Font744:
            result = fmcFont744;
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
            Logger::getInstance()->error("Fonts directory does not exist: %s\n", fontsDirectory.c_str());
        }
    } catch (const std::filesystem::filesystem_error &e) {
        Logger::getInstance()->error("Error reading fonts directory: %s\n", e.what());
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

        // The text-grid origin lives in an 8-param control block laid out as:
        //   .. 08 00 00 00 <left> 00 <top> 00 0e 00 18 00
        // (0e/18 = the 14x24 character grid). Its position within the row is
        // not fixed: most fonts (737, 744, default, xcrafts) prefix an extra
        // control block, pushing the values past the indices the old fixed
        // check used, so the margin was only ever patched for the Airbus/VGA
        // fonts. Locate the block by signature instead (sniffed defaults are the
        // MCDU values: left 0x34, top 0x25). The position itself comes from the
        // per-hardware screen-layout config so it lives in one place; the FMC also
        // re-asserts it via setScreenPosition after the upload.
        const FMCScreenLayout layout = FMCHardwareMapping::ScreenLayoutForHardware(hardwareType);
        for (size_t i = 0; i + 11 < row.size(); i++) {
            if (row[i] == 0x08 && row[i + 1] == 0x00 && row[i + 2] == 0x00 && row[i + 3] == 0x00 &&
                row[i + 4] == 0x34 && row[i + 5] == 0x00 && row[i + 6] == 0x25 && row[i + 7] == 0x00 &&
                row[i + 8] == 0x0e && row[i + 9] == 0x00 && row[i + 10] == 0x18 && row[i + 11] == 0x00) {
                row[i + 4] = static_cast<unsigned char>(36 + layout.x); // left
                row[i + 6] = static_cast<unsigned char>(20 + layout.y); // top
            }
        }
    }
}

namespace {
// Glyph-cell padding re-serializer. The font upload format
// (reverse-engineered from SimAppPro captures) is, per glyph set:
//   - a GEOMETRY block declaring width/height/datalen (datalen = 4 + height*3),
//   - the glyph buffer (one datalen-byte record per glyph) streamed in 512-byte
//     WRITE blocks, each carrying <cumulativeOffset:4><length:4> + data and
//     followed by a COMMIT block, wrapped in f0 00 .. 3c/12 packets with f0 01
//     flush packets; sets are separated by 0x02 reset packets.
// Block addresses are transfer bookkeeping (the device positions glyphs by the
// datalen stride), so we regenerate them with a simple consistent scheme.

void putU32(std::vector<unsigned char> &b, size_t off, uint32_t v) {
    if (off + 4 > b.size()) {
        return;
    }
    b[off] = static_cast<unsigned char>(v & 0xff);
    b[off + 1] = static_cast<unsigned char>((v >> 8) & 0xff);
    b[off + 2] = static_cast<unsigned char>((v >> 16) & 0xff);
    b[off + 3] = static_cast<unsigned char>((v >> 24) & 0xff);
}

uint32_t readU32(const std::vector<unsigned char> &b, size_t off) {
    if (off + 4 > b.size()) {
        return 0;
    }
    return static_cast<uint32_t>(b[off]) | (static_cast<uint32_t>(b[off + 1]) << 8) |
           (static_cast<uint32_t>(b[off + 2]) << 16) | (static_cast<uint32_t>(b[off + 3]) << 24);
}

// Returns the control-block command low byte (0x05/0x06/0x07) if a font control
// block (<id> bb 00 00 <cmd> 01 ..) starts at p[i], otherwise -1. Identifier
// agnostic so it works before or after convertGlyphDataForHardware.
int controlBlockCmd(const std::vector<unsigned char> &p, size_t i) {
    if (i + 14 > p.size()) {
        return -1;
    }
    if (p[i + 1] == 0xbb && p[i + 2] == 0x00 && p[i + 3] == 0x00 && p[i + 5] == 0x01 &&
        (p[i + 4] == 0x05 || p[i + 4] == 0x06 || p[i + 4] == 0x07)) {
        return p[i + 4];
    }
    return -1;
}

// Parses one segment's packets into its geometry block (42 bytes) and the
// concatenated glyph buffer, using the editor's proven skip logic. Captures the
// WRITE (29B, from 0x3c) and COMMIT (17B, from 0x12) block templates if unseen.
bool parseGlyphSegment(const std::vector<std::vector<unsigned char>> &packets,
    std::vector<unsigned char> &geoOut, std::vector<unsigned char> &bufOut,
    std::vector<unsigned char> &writeTpl, std::vector<unsigned char> &commitTpl,
    std::vector<uint32_t> &writeAddrs) {
    bufOut.clear();
    writeAddrs.clear();
    bool haveGeo = false;
    int bytesToSkip = 0;

    for (const auto &p : packets) {
        if (p.size() < 4 || p[0] != 0xf0 || p[1] != 0x00) {
            continue;
        }
        unsigned char type = p[3];

        if (type == 0x2a) { // standalone geometry packet (large set), no glyph data
            for (size_t i = 4; i + 14 <= p.size(); i++) {
                if (controlBlockCmd(p, i) == 0x06) {
                    geoOut.assign(p.begin() + i, p.begin() + std::min(p.size(), i + 42));
                    haveGeo = true;
                    break;
                }
            }
            continue;
        }

        if (type == 0x12) { // carries one glyph byte + a COMMIT block
            if (p.size() > 4) {
                bufOut.push_back(p[4]);
            }
            if (commitTpl.empty() && p.size() >= 22 && controlBlockCmd(p, 5) == 0x05) {
                commitTpl.assign(p.begin() + 5, p.begin() + 22);
            }
            continue;
        }

        if (type != 0x3c) {
            continue;
        }

        size_t i = 4, n = p.size();
        while (i < n) {
            if (bytesToSkip > 0) {
                bytesToSkip--;
                i++;
                continue;
            }
            int cmd = controlBlockCmd(p, i);
            if (cmd >= 0) {
                if (cmd == 0x06 && geoOut.empty() && i + 42 <= n) {
                    // Small set's geometry is fused into a 0x3c packet, not a
                    // standalone 0x2a packet like the large set.
                    geoOut.assign(p.begin() + i, p.begin() + i + 42);
                    haveGeo = true;
                }
                if (cmd == 0x07) {
                    if (writeTpl.empty() && i + 29 <= n) {
                        writeTpl.assign(p.begin() + i, p.begin() + i + 29);
                    }
                    writeAddrs.push_back(readU32(p, i + 8)); // descriptor-table pointer for this chunk
                }
                int total = 17 + p[i + 13];
                if (i + static_cast<size_t>(total) <= n) {
                    i += total;
                } else {
                    bytesToSkip = total - static_cast<int>(n - i);
                    i = n;
                }
                continue;
            }
            bufOut.push_back(p[i]);
            i++;
        }
    }

    return haveGeo && geoOut.size() >= 29;
}

// Runtime cell-height resizer. Takes the authored 29px font and PADS every glyph with
// blank rows up to newHeight (native ink -> native size; the extra rows become row
// pitch). Re-serializes the two glyph sets in the device's RESIZE format (verified
// against SAP size_30/31/32 and capture_x16_y17_w23_h29-34-29):
//   * per-set geometry + WRITE chunks, each WRITE carrying the set selector at byte@17
//     (0x05 for set0, 0x06 for set1). The 0x05->0x06 transition is what commits set0.
//   * NO 0x02 reset packets — those belong to the h29 baseline only; splicing them into
//     a resize wedges the device on re-upload. See the note at the bottom.
bool padGlyphsToHeight(std::vector<std::vector<unsigned char>> &data, unsigned char newHeight, unsigned char newWidth) {
    constexpr unsigned int kOldHeight = 29;
    constexpr unsigned int kOldDataLen = 91; // 4 + 29*3
    constexpr unsigned int kChunk = 512;
    const unsigned int newDataLen = 4 + static_cast<unsigned int>(newHeight) * 3;

    if (newHeight <= kOldHeight) {
        // Nothing to do at/below the authored height; leave the font untouched
        // so the caller sends the normal 29px font (acts as a reset).
        Logger::getInstance()->info("Font::ResizeCellHeight: cell height %u <= %u, sending font unmodified\n", newHeight, kOldHeight);
        return true;
    }
    // PAD each glyph with blank rows to the new height: keep the 29 original ink rows
    // byte-for-byte and append blank (zero) rows at the bottom. The device renders glyph
    // ink at native bitmap-pixel size, so the character keeps its size and the extra
    // cell height becomes pure row pitch below the glyph.
    const unsigned int botPad = newHeight - kOldHeight;

    std::vector<unsigned char> flushTpl, resetTpl;
    std::vector<size_t> resetIdx;
    for (size_t k = 0; k < data.size(); k++) {
        const auto &p = data[k];
        if (p.size() >= 4 && p[0] == 0xf0 && p[1] == 0x00 && p[3] == 0x02) {
            resetIdx.push_back(k);
            if (resetTpl.empty()) {
                resetTpl = p;
            }
        }
        if (p.size() >= 4 && p[0] == 0xf0 && p[1] == 0x01 && flushTpl.empty()) {
            flushTpl = p;
        }
    }
    if (resetIdx.size() < 2 || flushTpl.empty()) {
        Logger::getInstance()->error("Font::ResizeCellHeight: unexpected font structure (resets=%zu)\n", resetIdx.size());
        return false;
    }

    // default.h's two 0x02 packets are only used HERE to split the authored font into
    // set0 = [0, reset0), set1 = (reset0, reset1), suffix = [reset1, end). We do NOT
    // re-emit the 0x02 packets (the resize format omits them, see below); the suffix is
    // replayed with its 0x02 stripped.
    std::vector<std::vector<unsigned char>> seg0(data.begin(), data.begin() + resetIdx[0]);
    std::vector<std::vector<unsigned char>> seg1(data.begin() + resetIdx[0] + 1, data.begin() + resetIdx[1]);
    std::vector<std::vector<unsigned char>> suffix(data.begin() + resetIdx[1], data.end());

    std::vector<unsigned char> writeTpl, commitTpl, geo0, buf0, geo1, buf1;
    std::vector<uint32_t> writeAddrs0, writeAddrs1;
    if (!parseGlyphSegment(seg0, geo0, buf0, writeTpl, commitTpl, writeAddrs0) ||
        !parseGlyphSegment(seg1, geo1, buf1, writeTpl, commitTpl, writeAddrs1)) {
        Logger::getInstance()->error("Font::ResizeCellHeight: failed to parse glyph segments\n");
        return false;
    }
    if (writeTpl.size() != 29 || commitTpl.size() != 17) {
        Logger::getInstance()->error("Font::ResizeCellHeight: missing WRITE/COMMIT block templates\n");
        return false;
    }

    unsigned char seq = 0;
    auto makePacket = [&](unsigned char type, const unsigned char *payload, size_t plen) {
        std::vector<unsigned char> pk(64, 0);
        pk[0] = 0xf0;
        pk[1] = 0x00;
        pk[2] = seq++;
        pk[3] = type;
        for (size_t i = 0; i < plen && i < 60; i++) {
            pk[4 + i] = payload[i];
        }
        return pk;
    };

    auto emitSet = [&](const std::vector<unsigned char> &geoTpl, const std::vector<unsigned char> &buf,
                       const std::vector<uint32_t> &origAddrs, std::vector<std::vector<unsigned char>> &out) {
        // Patch geometry: width @21, height @23 (row pitch), datalen @25 (bytes
        // per glyph record), and total-size @37 (= glyphCount*datalen + const).
        std::vector<unsigned char> geo = geoTpl;
        uint32_t glyphCount = readU32(geo, 29);
        uint32_t oldDatalen = readU32(geo, 25);
        uint32_t sizeField = readU32(geo, 37);
        uint32_t sizeConst = sizeField >= glyphCount * oldDatalen ? sizeField - glyphCount * oldDatalen : 0;
        geo[21] = static_cast<unsigned char>(newWidth & 0xff);
        geo[22] = static_cast<unsigned char>((newWidth >> 8) & 0xff);
        geo[23] = static_cast<unsigned char>(newHeight & 0xff);
        geo[24] = static_cast<unsigned char>((newHeight >> 8) & 0xff);
        putU32(geo, 25, newDataLen);
        putU32(geo, 37, glyphCount * newDataLen + sizeConst);

        // addr@8 is a per-glyph descriptor-table pointer (~7.8 bytes/glyph, and
        // VARIABLE width). The descriptor table is the same for any cell height
        // (same 116 glyphs), so a chunk's correct address is the ORIGINAL
        // descriptor address of that chunk's FIRST glyph. The resized chunks start
        // at different glyphs than the original (datalen 91 vs newDataLen), so we
        // interpolate PIECEWISE between the original chunk-boundary addresses.
        // (A single linear span across the set lands the digit descriptors ~13
        // bytes off and blanks them - the "numeric characters removed" bug.)
        // origAddrs[j] is the descriptor address at original glyph floor(j*512/91).
        auto descAddrForGlyph = [&](uint32_t g) -> uint32_t {
            if (origAddrs.empty()) {
                return 0;
            }
            for (size_t j = 0; j + 1 < origAddrs.size(); j++) {
                uint32_t gj = static_cast<uint32_t>(j * kChunk / oldDatalen);
                uint32_t gj1 = static_cast<uint32_t>((j + 1) * kChunk / oldDatalen);
                if (g >= gj && g < gj1 && gj1 > gj) {
                    return origAddrs[j] + (origAddrs[j + 1] - origAddrs[j]) * (g - gj) / (gj1 - gj);
                }
            }
            return origAddrs.back(); // at/after the last original chunk: cap in-region
        };

        // Build each glyph record at the new height: keep the 29 original ink rows
        // byte-for-byte, then append blank (zero) rows below. The device renders ink at
        // native pixel size, so the character keeps its size and the extra rows become
        // row pitch below the glyph.
        unsigned int nrec = static_cast<unsigned int>(buf.size() / kOldDataLen);
        std::vector<unsigned char> nb;
        nb.reserve(static_cast<size_t>(glyphCount) * newDataLen);
        for (unsigned int g = 0; g < nrec; g++) {
            const unsigned char *rec = &buf[g * kOldDataLen];
            nb.insert(nb.end(), rec, rec + 4); // codepoint + pad (header unchanged)
            const unsigned char *bitmap = rec + 4;
            nb.insert(nb.end(), bitmap, bitmap + kOldHeight * 3);    // original 29 rows
            nb.insert(nb.end(), static_cast<size_t>(botPad) * 3, 0); // blank bottom rows
        }
        // Pad to exactly glyphCount records: the source parses 115 but geometry
        // declares 116. The 116th record is blank in the original.
        size_t targetLen = static_cast<size_t>(glyphCount) * newDataLen;
        if (nb.size() < targetLen) {
            nb.resize(targetLen, 0);
        }
        const unsigned int realLen = static_cast<unsigned int>(nb.size());

        // Emit this set's geometry as a standalone 0x2a packet.
        std::vector<unsigned char> geoPayload(60, 0);
        for (size_t i = 0; i < geo.size() && i < 60; i++) {
            geoPayload[i] = geo[i];
        }
        out.push_back(makePacket(0x2a, geoPayload.data(), 60));

        // One unit per chunk: WRITE + exactly `length` glyph bytes + COMMIT,
        // wrapped in the authored 0x3c packetisation (see below).
        unsigned int nchunks = (realLen + kChunk - 1) / kChunk; // ceil; only the last chunk may be partial
        for (unsigned int j = 0; j < nchunks; j++) {
            unsigned int off = j * kChunk;
            unsigned int length = std::min(kChunk, realLen - off);
            bool isLast = (j + 1 == nchunks);

            uint32_t addr = descAddrForGlyph(static_cast<uint32_t>(off / newDataLen));

            std::vector<unsigned char> w = writeTpl;
            putU32(w, 8, addr);         // descriptor-table pointer
            putU32(w, 13, length + 12); // block byte-count (length + 12 header bytes); NOT a constant
            putU32(w, 21, off);         // cumulative offset into the glyph buffer
            putU32(w, 25, length);      // bytes the device consumes from this chunk
            // Byte @17 is the per-set font-id discriminator: 0x05 for set0, 0x06 for
            // set1, matching that set's GEOMETRY block (geo[17]) and the finalization
            // suffix. writeTpl is captured ONCE from set0 (@17=0x05) and reused for
            // both sets, so set1's WRITE blocks would otherwise stream under id 0x05
            // while set1's geometry/commit reference 0x06 — an inconsistent set1
            // transaction that wedges the device on re-upload. Force it from geo[17].
            w[17] = geo[17];

            std::vector<unsigned char> c = commitTpl;
            putU32(c, 8, addr);

            std::vector<unsigned char> unit;
            unit.insert(unit.end(), w.begin(), w.end());
            unit.insert(unit.end(), nb.begin() + off, nb.begin() + off + length);
            unit.insert(unit.end(), c.begin(), c.end());

            // Wrapping rule: U bytes -> floor(U/60) packets of TYPE=0x3c, then if
            // U%60 > 0, one packet of TYPE=(U%60). Flush after EVERY chunk including
            // the last. No 0x02 reset packets anywhere in a resize stream.
            if (!isLast) {
                unit.resize(600, 0);
                for (int k = 0; k < 9; k++) {
                    out.push_back(makePacket(0x3c, unit.data() + k * 60, 60));
                }
                out.push_back(makePacket(0x12, unit.data() + 540, 60));
                for (int f = 0; f < 3; f++) {
                    out.push_back(flushTpl);
                }
            } else {
                size_t rawLen = unit.size();
                while (unit.size() % 60 != 0) unit.push_back(0);
                size_t npackets = unit.size() / 60;
                for (size_t k = 0; k + 1 < npackets; k++) {
                    out.push_back(makePacket(0x3c, unit.data() + k * 60, 60));
                }
                unsigned char lastType = rawLen % 60 == 0 ? 0x3c : static_cast<unsigned char>(rawLen % 60);
                out.push_back(makePacket(lastType, unit.data() + (npackets - 1) * 60, 60));
                for (int f = 0; f < 3; f++) out.push_back(flushTpl);
            }
        }
    };

    // NO 0x02 reset packets anywhere. Every real device RESIZE capture (size_30/31/32
    // and capture_x16_y17_w23_h29-34-29 upload 2) contains ZERO 0x02 — only the h29
    // baseline uses them. A resize commits set0 implicitly when the WRITE set-selector
    // (byte@17) transitions 0x05 -> 0x06 at set1; the 0x02 splice was a band-aid that
    // also format-mixes the h29 stream into a resize and wedges the device on re-upload
    // (the "hangs after one font" brick). With the @17 selector fix above, set1's WRITE
    // blocks correctly carry 0x06, so the 5->6 transition triggers the set0 commit and
    // no 0x02 is needed. resetTpl is intentionally unused now (flushTpl is still used
    // by emitSet for the per-chunk flushes).
    (void) resetTpl;
    std::vector<std::vector<unsigned char>> result;
    emitSet(geo0, buf0, writeAddrs0, result);
    emitSet(geo1, buf1, writeAddrs1, result);
    for (const auto &p : suffix) {
        if (p.size() >= 4 && p[0] == 0xf0 && p[1] == 0x00 && p[3] == 0x02) continue;
        result.push_back(p);
    }

    Logger::getInstance()->info("Font::ResizeCellHeight: rebuilt font at cell height %u (datalen %u), %zu packets\n",
        static_cast<unsigned int>(newHeight), newDataLen, result.size());
    data.swap(result);
    return true;
}
} // namespace

bool Font::ResizeCellHeight(std::vector<std::vector<unsigned char>> &data, unsigned char cellHeight, unsigned char cellWidth) {
    constexpr unsigned char kDefaultHeight = 29;
    constexpr unsigned char kDefaultWidth  = 23;

    if (cellHeight == 0) cellHeight = kDefaultHeight;
    if (cellWidth  == 0) cellWidth  = kDefaultWidth;

    return padGlyphsToHeight(data, cellHeight, cellWidth);
}

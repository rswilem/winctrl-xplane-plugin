#ifndef PAP3MCP_LCD_SEGMENTS_H
#define PAP3MCP_LCD_SEGMENTS_H

#include <algorithm>
#include <array>
#include <cstdint>

namespace pap3mcp::lcd {

    // LCD payload size (0x19..0x38 inclusive = 32 bytes)
    inline constexpr int kPayloadSize = 32;
    using Payload = std::array<uint8_t, kPayloadSize>;

    // Group offsets ordered as [Mid, TopL, BotL, Bot, BotR, TopR, Top]
    struct GroupOffsets {
            uint8_t mid, topL, botL, bot, botR, topR, top;
    };

    // Four segment groups (absolute offsets)
    inline constexpr GroupOffsets G0{0x1D, 0x21, 0x25, 0x29, 0x2D, 0x31, 0x35}; // SPD + CAPT_CRS
    inline constexpr GroupOffsets G1{0x1E, 0x22, 0x26, 0x2A, 0x2E, 0x32, 0x36}; // HDG + ALT(hi)
    inline constexpr GroupOffsets G2{0x1F, 0x23, 0x27, 0x2B, 0x2F, 0x33, 0x37}; // VSPD + ALT(lo)
    inline constexpr GroupOffsets G3{0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38}; // FO_CRS

    // G0 flags (SPD & CAPT_CRS)
    inline constexpr uint8_t SPD_UNITS = 0x01;
    inline constexpr uint8_t SPD_TENS = 0x02;
    inline constexpr uint8_t SPD_HUNDREDS = 0x04;
    inline constexpr uint8_t SPD_KILO = 0x08;
    inline constexpr uint8_t CPT_CRS_UNITS = 0x20;
    inline constexpr uint8_t CPT_CRS_TENS = 0x40;
    inline constexpr uint8_t CPT_CRS_HUNDREDS = 0x80;

    // G1 flags (HDG & ALT high)
    inline constexpr uint8_t ALT_HUNDREDS = 0x01;
    inline constexpr uint8_t ALT_KILO = 0x02;
    inline constexpr uint8_t ALT_TENS_KILO = 0x04;
    inline constexpr uint8_t HDG_UNITS = 0x10;
    inline constexpr uint8_t HDG_TENS = 0x20;
    inline constexpr uint8_t HDG_HUNDREDS = 0x40;

    // G2 flags (VSPD & ALT low)
    inline constexpr uint8_t VSPD_UNITS = 0x01;
    inline constexpr uint8_t VSPD_TENS = 0x02;
    inline constexpr uint8_t VSPD_HUNDREDS = 0x04;
    inline constexpr uint8_t VSPD_KILO = 0x08;
    inline constexpr uint8_t ALT_UNITS = 0x40;
    inline constexpr uint8_t ALT_TENS = 0x80;

    // G3 flags (FO_CRS)
    inline constexpr uint8_t FO_CRS_UNITS = 0x10;
    inline constexpr uint8_t FO_CRS_TENS = 0x20;
    inline constexpr uint8_t FO_CRS_HUNDREDS = 0x40;

    // Absolute offsets for dots/labels
    inline constexpr uint8_t OFF_19 = 0x19;
    inline constexpr uint8_t OFF_1A = 0x1A;
    inline constexpr uint8_t OFF_1B = 0x1B;
    inline constexpr uint8_t OFF_1C = 0x1C;
    inline constexpr uint8_t OFF_1E = 0x1E;
    inline constexpr uint8_t OFF_1F = 0x1F;
    inline constexpr uint8_t OFF_22 = 0x22;
    inline constexpr uint8_t OFF_26 = 0x26;
    inline constexpr uint8_t OFF_28 = 0x28;
    inline constexpr uint8_t OFF_2A = 0x2A;
    inline constexpr uint8_t OFF_2C = 0x2C;
    inline constexpr uint8_t OFF_2E = 0x2E;
    inline constexpr uint8_t OFF_32 = 0x32;
    inline constexpr uint8_t OFF_34 = 0x34;
    inline constexpr uint8_t OFF_36 = 0x36;
    inline constexpr uint8_t OFF_38 = 0x38;

    // Dots
    inline constexpr uint8_t DOT_SPD_KILO = 0x08; // Decimal point between KILO and HUNDREDS (for MACH "0.XXX" / ".XXX")
    inline constexpr uint8_t DOT_SPD = 0x04;      // Decimal point between HUNDREDS and TENS
    inline constexpr uint8_t DOT_CPT_CRS = 0x20;
    inline constexpr uint8_t DOT_HDG = 0x08;
    inline constexpr uint8_t DOT_ALT = 0x01;
    inline constexpr uint8_t DOT_VSPD = 0x04;
    inline constexpr uint8_t DOT_FO_CRS = 0x10;

    // SPD bars
    inline constexpr uint8_t SPD_BAR_BOTTOM = 0x80;
    inline constexpr uint8_t SPD_BAR_TOP = 0x80;

    // VSPD signs
    inline constexpr uint8_t VSPD_MINUS = 0x10;
    inline constexpr uint8_t VSPD_PLUS_BOT = 0x80;
    inline constexpr uint8_t VSPD_PLUS_TOP = 0x80;

    // Labels
    inline constexpr uint8_t LBL_FPA = 0x80;
    inline constexpr uint8_t LBL_VS = 0x80;
    inline constexpr uint8_t LBL_HDG_L = 0x08;
    inline constexpr uint8_t LBL_HDG_R = 0x08;
    inline constexpr uint8_t LBL_TRK_L = 0x08;
    inline constexpr uint8_t LBL_TRK_R = 0x08;
    inline constexpr uint8_t LBL_IAS = 0x80;
    inline constexpr uint8_t LBL_MACH_L = 0x80;
    inline constexpr uint8_t LBL_MACH_R = 0x80;

    // Seven-segment bit mapping: A=top, B=top-right, C=bottom-right, D=bottom, E=bottom-left, F=top-left, G=middle
    static constexpr uint8_t A = 1u << 0, B = 1u << 1, C = 1u << 2, D = 1u << 3, E = 1u << 4, F = 1u << 5, G = 1u << 6;

    static constexpr uint8_t kDigitMask[10] = {
        /*0*/ (A | B | C | D | E | F),
        /*1*/ B | C,
        /*2*/ A | B | G | E | D,
        /*3*/ A | B | C | D | G,
        /*4*/ F | G | B | C,
        /*5*/ A | F | G | C | D,
        /*6*/ A | F | E | D | C | G,
        /*7*/ A | B | C,
        /*8*/ A | B | C | D | E | F | G,
        /*9*/ (A | B | C | D | F | G)};

    static constexpr uint8_t kLetterA = (A | B | C | E | F | G);

    // Helper functions
    inline int idx(uint8_t absOff) {
        return static_cast<int>(absOff) - 0x19;
    }

    inline void applyOnePos(Payload &p, uint8_t absOff, uint8_t flag, bool on) {
        if (!on) {
            return;
        }
        const int i = idx(absOff);
        if (i < 0 || i >= kPayloadSize) {
            return;
        }
        p[static_cast<size_t>(i)] |= flag;
    }

    inline void drawDigit(const GroupOffsets &g, Payload &p, uint8_t flag, int digit) {
        digit = std::clamp(digit, 0, 9);
        const uint8_t m = kDigitMask[digit];
        applyOnePos(p, g.mid, flag, (m & G) != 0);
        applyOnePos(p, g.topL, flag, (m & F) != 0);
        applyOnePos(p, g.botL, flag, (m & E) != 0);
        applyOnePos(p, g.bot, flag, (m & D) != 0);
        applyOnePos(p, g.botR, flag, (m & C) != 0);
        applyOnePos(p, g.topR, flag, (m & B) != 0);
        applyOnePos(p, g.top, flag, (m & A) != 0);
    }

    inline void drawLetterA(const GroupOffsets &g, Payload &p, uint8_t flag) {
        applyOnePos(p, g.mid, flag, (kLetterA & G) != 0);
        applyOnePos(p, g.topL, flag, (kLetterA & F) != 0);
        applyOnePos(p, g.botL, flag, (kLetterA & E) != 0);
        applyOnePos(p, g.bot, flag, (kLetterA & D) != 0);
        applyOnePos(p, g.botR, flag, (kLetterA & C) != 0);
        applyOnePos(p, g.topR, flag, (kLetterA & B) != 0);
        applyOnePos(p, g.top, flag, (kLetterA & A) != 0);
    }

    inline void setFlag(Payload &p, uint8_t absOffset, uint8_t mask, bool enable) {
        const int i = idx(absOffset);
        if (i < 0 || i >= kPayloadSize) {
            return;
        }
        if (enable) {
            p[static_cast<size_t>(i)] |= mask;
        }
    }

    inline void digits3(int value, int &h, int &t, int &u) {
        value = std::clamp(value, 0, 999);
        h = (value / 100) % 10;
        t = (value / 10) % 10;
        u = value % 10;
    }

    inline void digits4(int value, int &k, int &h, int &t, int &u) {
        value = std::clamp(value, 0, 9999);
        k = (value / 1000) % 10;
        h = (value / 100) % 10;
        t = (value / 10) % 10;
        u = value % 10;
    }

    inline void digits5(int value, int &t10k, int &k, int &h, int &t, int &u) {
        value = std::clamp(value, 0, 99999);
        t10k = (value / 10000) % 10;
        k = (value / 1000) % 10;
        h = (value / 100) % 10;
        t = (value / 10) % 10;
        u = value % 10;
    }

    // Dash drawing functions - draw middle segment (G) for dash character
    inline void drawDash(const GroupOffsets &g, Payload &p, uint8_t flag) {
        applyOnePos(p, g.mid, flag, true);
    }

    inline void drawSpdDashes(Payload &p) {
        drawDash(G0, p, SPD_HUNDREDS);
        drawDash(G0, p, SPD_TENS);
        drawDash(G0, p, SPD_UNITS);
    }

    inline void drawHdgDashes(Payload &p) {
        drawDash(G1, p, HDG_HUNDREDS);
        drawDash(G1, p, HDG_TENS);
        drawDash(G1, p, HDG_UNITS);
    }

    inline void drawVviDashes(Payload &p) {
        drawDash(G2, p, VSPD_KILO);
        drawDash(G2, p, VSPD_HUNDREDS);
        drawDash(G2, p, VSPD_TENS);
        drawDash(G2, p, VSPD_UNITS);
    }

}

#endif

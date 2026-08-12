#ifndef DISPLAY_FRAME_H
#define DISPLAY_FRAME_H

#include <cstdint>
#include <vector>

// ===================================================================
// DISPLAY FRAME PROTOCOL (0xF0 frame family)
// ===================================================================
// Shared by AGP, TCAS, RMP, FCU/EFIS and URSA Minor Throttle displays.
// Every LCD update is a two-step transaction: a data frame followed by a
// commit frame, both 64-byte HID output reports sharing one template:
//
//   [0..1]  0xF0 0x00              frame magic
//   [2]     packetNumber           rolling counter, 1..255
//   [3]     opcode                 data opcode (device specific) or 0x11 commit
//   [4]     identifierByte         device/board identifier
//   [5]     familyMarker           protocol family (0xBB / 0xB9 / 0xBF)
//   [8]     0x02 data / 0x03 commit
//   [9]     0x01
//   [12..13] 0xFF 0xFF
//   [14]    extendedId             device specific (data frames only)
//   [17]    payloadTag             device specific (data frames only)
namespace DisplayFrame {

    // Build a 64-byte zeroed data frame with the header filled in.
    // Display payload bytes are appended starting at index 25 by the caller.
    inline std::vector<uint8_t> buildDataFrame(uint8_t packetNumber, uint8_t opcode, uint8_t identifierByte, uint8_t familyMarker, uint8_t extendedId = 0x00, uint8_t payloadTag = 0x24) {
        std::vector<uint8_t> packet(64, 0x00);
        packet[0] = 0xF0;
        packet[2] = packetNumber;
        packet[3] = opcode;
        packet[4] = identifierByte;
        packet[5] = familyMarker;
        packet[8] = 0x02;
        packet[9] = 0x01;
        packet[12] = 0xFF;
        packet[13] = 0xFF;
        packet[14] = extendedId;
        packet[17] = payloadTag;
        return packet;
    }

    // Build a 64-byte zeroed commit frame (opcode 0x11) that flushes the
    // previously sent data frame to the display.
    inline std::vector<uint8_t> buildCommitFrame(uint8_t packetNumber, uint8_t identifierByte, uint8_t familyMarker, uint8_t extendedId = 0x00) {
        std::vector<uint8_t> packet(64, 0x00);
        packet[0] = 0xF0;
        packet[2] = packetNumber;
        packet[3] = 0x11;
        packet[4] = identifierByte;
        packet[5] = familyMarker;
        packet[8] = 0x03;
        packet[9] = 0x01;
        packet[12] = 0xFF;
        packet[13] = 0xFF;
        packet[14] = extendedId;
        return packet;
    }

    // Advance the rolling frame counter, skipping zero.
    inline void advancePacketNumber(uint8_t &packetNumber) {
        if (++packetNumber == 0) {
            packetNumber = 1;
        }
    }

}

#endif

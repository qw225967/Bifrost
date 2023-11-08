#include "webrtc_mask_generator.h"
#include <algorithm>
#include <math.h>
PacketMaskTable::PacketMaskTable(FecMaskType fec_mask_type,
    int num_media_packets)
    :table_(PickTable(fec_mask_type, num_media_packets)) {}

PacketMaskTable::~PacketMaskTable() = default;


rtc::ArrayView<const uint8_t> PacketMaskTable::LookUp(int num_media_packets,
    int num_fec_packets) {

    assert(num_media_packets > 0);
    assert(num_fec_packets > 0);
    assert(num_media_packets <= static_cast<int>(kMaxMediaPackets));
    assert(num_fec_packets <= num_media_packets);

    //对于被保护的媒体数<=12的情况，直接从随机或者突发表中获取掩码表即可
    if (num_media_packets <= 12) {
        return LookUpInFecTable(table_, num_media_packets - 1, num_fec_packets - 1);
    }

    //对于其他情况，首先确定需要的掩码字节数（1个fec packet的掩码字节数）
    //对于大于48个media packets的情况，直接使用交织模式（相当于1-D交织模式，flexfec中的这种模式）
    int mask_length =
        static_cast<int>(PacketMaskSize(static_cast<size_t>(num_media_packets)));

    // Generate FEC code mask for {num_media_packets(M), num_fec_packets(N)} (use
    // N FEC packets to protect M media packets) In the mask, each FEC packet
    // occupies one row, each bit / coloumn represent one media packet. E.g. Row
    // A, Col/Bit B is set to 1, means FEC packet A will have protection for media
    // packet B.

    // Loop through each fec packet.
    //这个地方实现了flex fec中的交织模式
    /*
    *  num_fec_packets(N) = 4, num_media_packets =16
    *  0  4  8   12  %N=0, 被第0个fec packet保护
    *  1  5  9   13  %N=1，被第1个fec packet保护
    *  2  6  10  14  %N=2，被第1个fec packet保护
    *  3  7  11  15  %N=3，被第1个fec packet保护
    */
    for (int row = 0; row < num_fec_packets; row++) {
        // Loop through each fec code in a row, one code has 8 bits.
    // Bit X will be set to 1 if media packet X shall be protected by current
    // FEC packet. In this implementation, the protection is interleaved, thus
    // media packet X will be protected by FEC packet (X % N)
        for (int col = 0; col < mask_length; col++) {
            fec_packet_mask_[row * mask_length + col] =
                ((col * 8) % num_fec_packets == row && (col * 8) < num_media_packets
                    ? 0x80
                    : 0x00) |
                ((col * 8 + 1) % num_fec_packets == row &&
                    (col * 8 + 1) < num_media_packets
                    ? 0x40
                    : 0x00) |
                ((col * 8 + 2) % num_fec_packets == row &&
                    (col * 8 + 2) < num_media_packets
                    ? 0x20
                    : 0x00) |
                ((col * 8 + 3) % num_fec_packets == row &&
                    (col * 8 + 3) < num_media_packets
                    ? 0x10
                    : 0x00) |
                ((col * 8 + 4) % num_fec_packets == row &&
                    (col * 8 + 4) < num_media_packets
                    ? 0x08
                    : 0x00) |
                ((col * 8 + 5) % num_fec_packets == row &&
                    (col * 8 + 5) < num_media_packets
                    ? 0x04
                    : 0x00) |
                ((col * 8 + 6) % num_fec_packets == row &&
                    (col * 8 + 6) < num_media_packets
                    ? 0x02
                    : 0x00) |
                ((col * 8 + 7) % num_fec_packets == row &&
                    (col * 8 + 7) < num_media_packets
                    ? 0x01
                    : 0x00);
        }
    }
    return { &fec_packet_mask_[0],
        static_cast<size_t>(num_fec_packets * mask_length) };
}

// If `num_media_packets` is larger than the maximum allowed by `fec_mask_type`
// for the bursty type, or the random table is explicitly asked for, then the
// random type is selected. Otherwise the bursty table callback is returned.
const uint8_t* PacketMaskTable::PickTable(FecMaskType fec_mask_type,
    int num_media_packets) {
    assert(num_media_packets > 0);
    assert(static_cast<size_t>(num_media_packets) <= kMaxMediaPackets);

    if (fec_mask_type != kFecMaskRandom &&
        num_media_packets <= static_cast<int>(kPacketMaskBurstyTbl[0])) {
        return &kPacketMaskBurstyTbl[0];
    }
    //返回的是地址，不是第一个元素12
    return &kPacketMaskRandomTbl[0];
}

// This algorithm is tailored to look up data in the `kPacketMaskRandomTbl` and
// `kPacketMaskBurstyTbl` tables. These tables only cover fec code for up to 12
// media packets. Starting from 13 media packets, the fec code will be generated
// at runtime. The format of those arrays is that they're essentially a 3
// dimensional array with the following dimensions: * media packet
//   * Size for kPacketMaskRandomTbl: 12
//   * Size for kPacketMaskBurstyTbl: 12
// * fec index
//   * Size for both random and bursty table increases from 1 to number of rows.
//     (i.e. 1-48, or 1-12 respectively).
// * Fec data (what actually gets returned)
//   * Size for kPacketMaskRandomTbl: 2 bytes.
//     * For all entries: 2 * fec index (1 based)
//   * Size for kPacketMaskBurstyTbl: 2 bytes.
//     * For all entries: 2 * fec index (1 based)
rtc::ArrayView<const uint8_t> LookUpInFecTable(const uint8_t* table,
    int media_packet_index, int fec_index) {

    assert(media_packet_index < table[0]);

    // Skip over the table size.
    const uint8_t* entry = &table[1];

    uint8_t entry_size_increment = 2;  // 0-16 are 2 byte wide, then changes to 6.

    // Hop over un-interesting array entries.
    for (int i = 0; i < media_packet_index; ++i) {
        if (i == 16)
            entry_size_increment = 6;
        uint8_t count = entry[0];
        ++entry;  // skip over the count.
        for (int j = 0; j < count; ++j) {
            entry += entry_size_increment * (j + 1);  // skip over the data.
        }
    }

    if (media_packet_index == 16)
        entry_size_increment = 6;

    assert(fec_index < entry[0]);
    ++entry;  // Skip over the size.

    // Find the appropriate data in the second dimension.

    // Find the specific data we're looking for.
    for (int i = 0; i < fec_index; ++i)
        entry += entry_size_increment * (i + 1);  // skip over the data.

    size_t size = entry_size_increment * (fec_index + 1);
    return { &entry[0], size };
}

void GeneratePacketMasks(int num_media_packets,
    int num_fec_packets,
    PacketMaskTable* mask_table,
    uint8_t* packet_mask) {

    assert(num_media_packets > 0);
    assert(num_fec_packets > 0);
    assert(num_fec_packets <= num_media_packets);


    //const int num_mask_bytes = PacketMaskSize(num_media_packets);

    // Retrieve corresponding mask table directly:for equal-protection case.
    // Mask = (k,n-k), with protection factor = (n-k)/k,
    // where k = num_media_packets, n=total#packets, (n-k)=num_fec_packets.
    rtc::ArrayView<const uint8_t> mask =
        mask_table->LookUp(num_media_packets, num_fec_packets);
    memcpy(packet_mask, &mask[0], mask.size());
}

/* void Generate2DPacketMasks(int num_media_packets, uint8_t* packet_mask) {
    int row = 0;
    int col = 0;
    for (row = floor(sqrt(num_media_packets)); row > 0; row--) {
        if (num_media_packets % row == 0) {
            col = num_media_packets / row;
            return;
        }
    }
}
 */
//是使用2字节掩码还是6字节掩码
size_t PacketMaskSize(size_t num_sequence_numbers) {
    assert(num_sequence_numbers <= 8 * kFecPacketMaskSizeLBitSet);
    if (num_sequence_numbers > 8 * kFecPacketMaskSizeLBitClear) {
        return kFecPacketMaskSizeLBitSet;
    }
    return kFecPacketMaskSizeLBitClear;
}


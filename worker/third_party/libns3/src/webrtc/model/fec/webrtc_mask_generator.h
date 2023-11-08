#pragma once

#include "webrtc_mask_random_table.h"
#include "webrtc_mask_bursty_table.h"

#include "ns3/array_view.h"

constexpr size_t kMaxMediaPackets = 48;

constexpr size_t kFecPacketMaskSizeLBitClear = 2;
constexpr size_t kFecPacketMaskSizeLBitSet = 6;
constexpr size_t kFecPacketMaskMaxSize = 288;

constexpr size_t kFecMaxPacketMaskSize = kFecPacketMaskSizeLBitSet;

// Types for the FEC packet masks. The type `kFecMaskRandom` is based on a
// random loss model. The type `kFecMaskBursty` is based on a bursty/consecutive
// loss model. The packet masks are defined in
// modules/rtp_rtcp/fec_private_tables_random(bursty).h
enum FecMaskType {
	kFecMaskRandom,
	kFecMaskBursty,
	kFec2D,
	kFec1DCol,
	kFecReedSolomon,
};

class PacketMaskTable {
public:
	PacketMaskTable(FecMaskType fec_mask_type, int num_media_packets);
	~PacketMaskTable();

	rtc::ArrayView<const uint8_t> LookUp(int num_media_packets, int num_fec_packets);

private:
	static const uint8_t* PickTable(FecMaskType fec_mask_type, int num_media_packets);

	const uint8_t* table_;
	uint8_t fec_packet_mask_[kFecPacketMaskMaxSize];
};

rtc::ArrayView<const uint8_t> LookUpInFecTable(const uint8_t* table,
	int media_packet_index, int fec_index);

// Returns an array of packet masks. The mask of a single FEC packet
// corresponds to a number of mask bytes. The mask indicates which
// media packets should be protected by the FEC packet.

// \param[in]  num_media_packets       The number of media packets to protect.
//                                     [1, max_media_packets].
// \param[in]  num_fec_packets         The number of FEC packets which will
//                                     be generated. [1, num_media_packets].
// \param[in]  mask_table              An instance of the `PacketMaskTable`
//                                     class, which contains the type of FEC
//                                     packet mask used, and a pointer to the
//                                     corresponding packet masks.
// \param[out] packet_mask             A pointer to hold the packet mask array,
//                                     of size: num_fec_packets *
//                                     "number of mask bytes".

/*
*返回packet masks的数组，一个单个FEC packet的掩码对应一定数量的mask bytes
*掩码指明一个FEC packet保护了哪些media packets
*/
void GeneratePacketMasks(int num_media_packets,
	int num_fec_packets,
	PacketMaskTable* mask_table,
	uint8_t* packet_mask);

//void Generate2DPacketMasks(int num_media_packets, uint8_t* packet_mask);

// Returns the required packet mask size, given the number of sequence numbers
// that will be covered.
size_t PacketMaskSize(size_t num_sequence_numbers);


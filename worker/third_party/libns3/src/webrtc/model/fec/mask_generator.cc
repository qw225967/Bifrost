#include "mask_generator.h"
#include <assert.h>
#include <cstring>

MaskGenerator::MaskGenerator() {}
MaskGenerator::~MaskGenerator() {}

int MaskGenerator::NumFecPackets(int num_media_packets, int protection_factor) {
	// Result in Q0 with an unsigned round.
	int num_fec_packets = (num_media_packets * protection_factor + 50) / 100;
	// Generate at least one FEC packet if we need protection.
	if (protection_factor > 0 && num_fec_packets == 0) {
		num_fec_packets = 1;
	}
	assert(num_fec_packets <= num_media_packets);
	return num_fec_packets;
}

void MaskGenerator::GenerateMask(int num_media_packets, uint8_t protection_factor,
	FecMaskType fec_mask_type) {
	int num_fec_packets = NumFecPackets(num_media_packets, protection_factor);
	fec_packets_number_ = num_fec_packets;
	if (num_fec_packets == 0) {
		return;
	}
	//掩码表最长为 48 * 6 = 288个字节，因为一个fec packet的掩码最长为6个字节
	//最多不超过48个fec packets
	
	PacketMaskTable mask_table(fec_mask_type, num_media_packets);
	size_t packet_mask_size = PacketMaskSize(num_media_packets);
	per_fec_mask_size_ = packet_mask_size;
	std::memset(packet_masks, 0, num_fec_packets * packet_mask_size);
	GeneratePacketMasks(num_media_packets, num_fec_packets, &mask_table, packet_masks);
}

void MaskGenerator::GeneratorProtectionVector(int num_media_packets, uint8_t protection_factor, FecMaskType fec_mask_type) {
	protection_vector.clear();
	GenerateMask(num_media_packets, protection_factor, fec_mask_type);
	for (int i = 0; i < fec_packets_number_; i++) {
		std::vector<uint8_t> protection_indexs;
		for (int j = 0; j < per_fec_mask_size_; j++) {
			uint8_t data = packet_masks[i * per_fec_mask_size_ + j];
			if ((data & 0x80) == 0x80) {
				protection_indexs.push_back(8 * j + 0);
			}
			if ((data & 0x40) == 0x40) {
				protection_indexs.push_back(8 * j + 1);
			}
			if ((data & 0x20) == 0x20) {
				protection_indexs.push_back(8 * j + 2);
			}
			if ((data & 0x10) == 0x10) {
				protection_indexs.push_back(8 * j + 3);
			}
			if ((data & 0x08) == 0x08) {
				protection_indexs.push_back(8 * j + 4);
			}
			if ((data & 0x04) == 0x04) {
				protection_indexs.push_back(8 * j + 5);
			}
			if ((data & 0x02) == 0x02) {
				protection_indexs.push_back(8 * j + 6);
			}
			if ((data & 0x01) == 0x01) {
				protection_indexs.push_back(8 * j + 7);
			}

		}
		protection_vector.push_back(protection_indexs);
	}
}


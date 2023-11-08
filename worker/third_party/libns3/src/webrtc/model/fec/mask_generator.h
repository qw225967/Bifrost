#ifndef MASK_GENERATOR_H_
#define MASK_GENERATOR_H_
#include "webrtc_mask_generator.h"
#include <vector>
class MaskGenerator {
public:
	MaskGenerator();
	~MaskGenerator();	
	void GeneratorProtectionVector(int num_media_packets, uint8_t protection_factor, FecMaskType fec_mask_type);

	std::vector<std::vector<uint8_t>> ProtectionVector() { return protection_vector; }
private:
	int NumFecPackets(int num_media_packets, int protection_factor);
	void GenerateMask(int num_media_packets, uint8_t protection_factor, FecMaskType fec_mask_type);
	uint8_t packet_masks[kMaxMediaPackets * kFecMaxPacketMaskSize];
	int fec_packets_number_;
	int per_fec_mask_size_;
	std::vector<std::vector<uint8_t>> protection_vector;
	
};

#endif
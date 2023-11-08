#ifndef FLEX_FEC_SENDER_H_
#define FLEX_FEC_SENDER_H_

#include <stdint.h>
#include <vector>
#include <mutex>
#include "ns3/video_header.h"
#include "mask_generator.h"
#include "ns3/fec_header.h"
#include "fec_sender.h"
#include "ns3/transport_controller.h"

//根据packets数量和(random,burst)来产生掩码表
class FlexFecSender : public FecSender{
public:
	FlexFecSender(TransportController* transport_controller);
	~FlexFecSender();

	void SetProtectionFactor(int fec_rate) { protection_factor_ = fec_rate; }
	void AddVideoPacketAndGenerateFec(VideoHeader* packet);

	void SendFecPacket(uint8_t* buffer, size_t size);
private:
	mutable std::mutex mutex_;
	uint16_t frame_id_;
	std::vector<uint64_t> packets_seq_;
	uint8_t protection_factor_;
	std::unique_ptr<MaskGenerator> mask_generator_;
	std::unique_ptr<FecHeader> fec_header_;

	TransportController* transport_controller_;
}; 

#endif

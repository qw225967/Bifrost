#include "flex_fec_sender.h"
#include "ns3/packet.h"
#include <iostream>
const uint16_t kVideoPacketLength = 1480;

FlexFecSender::FlexFecSender(TransportController* transport_controller) : transport_controller_(transport_controller) {
	mask_generator_ = std::make_unique<MaskGenerator>();
	fec_header_ = std::make_unique<FecHeader>();
}
FlexFecSender::~FlexFecSender() {}

void FlexFecSender::AddVideoPacketAndGenerateFec(VideoHeader* packet) {
	packets_seq_.push_back(packet->Seq());
	//std::cout << "add video packet and generate fec a packet with seq = " << packet->seq << std::endl;
	if (packet->IsLastPacket()) {
		mask_generator_->GeneratorProtectionVector(packets_seq_.size(), 
			protection_factor_, FecMaskType::kFecMaskRandom);
		
		size_t fec_packet_cnt = mask_generator_->ProtectionVector().size();
		for (size_t i = 0; i < fec_packet_cnt; i++) {
			std::vector<uint16_t> protected_packets;
			//std::cout << "packets_seq_[0] = " << packets_seq_[0] << std::endl;
			for (size_t j = 0; j < mask_generator_->ProtectionVector()[i].size(); j++)
				protected_packets.push_back(packets_seq_[0] + mask_generator_->ProtectionVector()[i][j]);
			//ack_header_->GenerateFecPacket(frame_id_, i + 1, protected_packets);	
      fec_header_->SetFrameId(frame_id_);
      fec_header_->SetFecId(i + 1);
      fec_header_->SetProtectedPackets(protected_packets);	
		}
		packets_seq_.clear();
	}
	if (packet->IsFirstPacket()) {
		frame_id_ = packet->FrameId();
	}
}

void FlexFecSender::SendFecPacket(uint8_t* buffer, size_t size) {
	//kVideo
  auto packet = Create<Packet>();
	std::unique_ptr<PacketToSend> packet_to_send(new PacketToSend());
	packet_to_send->packet_type = PacketType::kForwardErrorCorrection;
	packet_to_send->packet = packet;
  packet_to_send->payload_size = 0;
	transport_controller_->paced_sender()->EnqueuePacket(std::move(packet_to_send));	
}




#include "flex_fec_receiver.h"

FlexFecReceiver::FlexFecReceiver(VideoPacketAssembler* assembler, AckHeader* ack_header)
	:video_packet_assembler_(assembler),
	ack_header_(ack_header){

	//fec_process_thread_ = std::thread(&FlexFecReceiver::ProcessFecPacket, this);
}
FlexFecReceiver::~FlexFecReceiver() {}

void FlexFecReceiver::OnFecPacket(FecHeader packet) {
	std::lock_guard<std::mutex> lock(fec_process_mutex_);
	//fec_packet_queue_.push_back(packet);	
	fec_packets_[packet.FrameId()][packet.FecId()] = packet.ProtectedPacketSeqs();
	FecDecode(packet.FrameId(), packet.FecId(), packet.ProtectedPacketSeqs());
}

void FlexFecReceiver::OnVideoPacket(uint16_t frame_id) {
	FecDecodeAll(frame_id);
}

void FlexFecReceiver::FecDecodeAll(uint16_t frame_id) {
	auto fec_packets = fec_packets_[frame_id];
	for (auto fec : fec_packets) {
		FecDecode(frame_id, fec.first, fec.second);
	}
}

//void FlexFecReceiver::ProcessFecPacket() {
//	FecPacket packet;
//	while (true) {
//		{
//			std::lock_guard<std::mutex> lock(fec_process_mutex_);
//			if (fec_packet_queue_.empty())
//				continue;
//			packet = fec_packet_queue_.front();
//			fec_packet_queue_.pop_front();
//		}
//		FecDecode(packet.frame_id, packet.fec_id, packet.protected_packets);
//		FecDecodeAll(packet.frame_id);
//		fec_packets_[packet.frame_id][packet.fec_id] = packet.protected_packets;
//	}
//}

//从算法上来看，的确使用mask是最快最有效的方法
void FlexFecReceiver::FecDecode(uint16_t frame_id, uint16_t fec_id, std::vector<uint16_t> protected_packets) {
	auto frame = video_packet_assembler_->GetFrameStatus(frame_id);
	if (frame == nullptr) {
		return;
	}
	uint64_t min_seq = frame->first_seq;
	uint8_t fec_received = 0;
	uint64_t loss_seq = 0;
	for (auto seq : protected_packets) {
		uint8_t index = seq - min_seq;
		if (frame->received_video_packet[index] == true) {
			fec_received++;
		}
		else {
			loss_seq = seq;
		}
	}
	if (fec_received == protected_packets.size() && loss_seq == 0) {
		fec_packets_[frame_id].erase(fec_id);
	}	
	else if (fec_received >= protected_packets.size() - 1 && loss_seq > 0) {
		//如果恢复出来了，则FecDecodeAll一遍，直到没有被恢复出来的
		video_packet_assembler_->OnRecoveryPacket(frame_id, loss_seq);
		ack_header_->OnRecoveryPacket(frame_id, loss_seq);
		fec_packets_[frame_id].erase(fec_id);
		FecDecodeAll(frame_id);		
	}
}

//https://www.51cto.com/article/718868.html  字节nack fec联合分析方法
//https://blog.csdn.net/qw225967/article/details/123405797  对比一下
#include "video_packet_assembler.h"
#include "ns3/flex_fec_receiver.h"
#include <iostream>
VideoPacketAssembler::VideoPacketAssembler(){}

VideoPacketAssembler::~VideoPacketAssembler() {}

void VideoPacketAssembler::OnVideoPacket(VideoHeader packet, uint64_t now_ms) {
	//std::lock_guard<std::recursive_mutex> lock(mutex_);
	Frame* frame_ptr = nullptr;	
	if (frames_.find(packet.FrameId()) == frames_.end()) {
		std::unique_ptr<Frame> frame = std::make_unique<Frame>();;
		frame->frame_id = packet.FrameId();
		frame->first_seq = packet.FirstSeq();		
		frame->total_packet_cnt = packet.PacketNumber();
		frame->received_video_packet.resize(frame->total_packet_cnt, false);
		//frame_ptr = frame.get();
		frames_[packet.FrameId()] = std::move(frame);		
	}	
    frame_ptr = frames_[packet.FrameId()].get();			
	frame_ptr->received_video_packet[packet.Seq() - frame_ptr->first_seq] = true;
	//std::cout << "VideoPacketAssembler::OnVideoPacket Seq = " << packet.seq << std::endl;
	flex_fec_receiver_->OnVideoPacket(frame_ptr->frame_id);
}

void VideoPacketAssembler::OnRecoveryPacket(uint16_t frame_id, uint64_t seq) {
	//std::lock_guard<std::recursive_mutex> lock(mutex_);
	auto frame_ptr = frames_[frame_id].get();
	frame_ptr->received_video_packet[seq - frame_ptr->first_seq] = true;	
	recover_packets_num_++;	
	//std::cout << "VideoPacketAssembler::OnRecoveryPacket Seq = " << seq << std::endl;
}

Frame* VideoPacketAssembler::GetFrameStatus(uint16_t frame_id)  {
	//std::lock_guard<std::recursive_mutex> lock(mutex_);
	if (frames_.find(frame_id) == frames_.end()) {
		return nullptr;
	}
	return frames_[frame_id].get();
}

void VideoPacketAssembler::ComputeLossRate() {
	uint64_t total_cnt = 0;
	uint64_t receive_cnt = 0;
	for (auto iter = frames_.begin(); iter != frames_.end(); ++iter) {
		total_cnt += iter->second->total_packet_cnt;
		int cnt = 0;
		for (auto rcv : iter->second->received_video_packet) {
			if (rcv == true) {
				cnt++;
			}
		}
		receive_cnt += cnt;
	}
	std::cout << "数据包总数为：" << total_cnt << " 本次的丢包率为 " << (total_cnt - receive_cnt) * 100 / total_cnt << " ;FEC恢复的丢包数量为" << recover_packets_num_ << std::endl;
}
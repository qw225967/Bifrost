#ifndef VIDEO_PACKET_ASSEMBLER_H_
#define VIDEO_PACKET_ASSEMBLER_H_
#include <map>
#include <vector>
#include <mutex>
#include "video_packet_observer.h"

class FlexFecReceiver;
struct Frame {
	uint16_t frame_id;
	uint64_t first_seq;
	/*uint16_t last_seq; */
	uint16_t total_packet_cnt;
	std::vector<bool> received_video_packet;
};

//每个fec recovery的seq也添加到这里来
class VideoPacketAssembler : public VideoPacketObserver {
	
public:
	VideoPacketAssembler();
	~VideoPacketAssembler();

	void OnVideoPacket(VideoHeader packet, uint64_t now_ms) override;
	void OnRecoveryPacket(uint16_t frame_id, uint64_t seq);
	Frame* GetFrameStatus(uint16_t frame_id); 
	void ComputeLossRate();

	void SetFlexFecReceiver(FlexFecReceiver* fec_receiver) { flex_fec_receiver_ = fec_receiver; }
private:	
	FlexFecReceiver* flex_fec_receiver_;
	std::map<uint16_t, std::unique_ptr<Frame>> frames_;	
	int recover_packets_num_ = 0;
	std::recursive_mutex mutex_;
};

#endif
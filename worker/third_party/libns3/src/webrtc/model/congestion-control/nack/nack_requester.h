#pragma once

#include <map>
#include <set>
#include <vector>
#include "nack_sender.h"
#include "ns3/sequence_number_util.h"
#include "ns3/video_header.h"

class NackRequester{
public:
	NackRequester(NackSender* nack_sender);
	~NackRequester();

	void ProcessNacks();
	int  OnReceivedPacket(uint16_t seq_num, bool is_keyframe);
	int  OnReceivedPacket(uint16_t seq_num, bool is_keyframe, bool is_recovered);

	void OnVideoPacket(VideoHeader& packet, uint64_t now_ms);


	void UpdateRtt(int64_t rtt_ms);
	void OnRtt(uint16_t rtt, uint64_t now_ms);

private:
	struct NackInfo {
		NackInfo();
		NackInfo(uint16_t seq_num, int64_t created_at_time);

		uint64_t seq_num;
		int64_t created_at_time;
		int64_t sent_at_time;
		int retries;
	};
	void ClearUpTo(uint16_t seq_num);
	void AddPacketsToNack(uint16_t seq_num_start, uint16_t seq_num_end);
	bool RemovePacketsUntilKeyFrame();
	std::vector<uint16_t> GetNackBatch();
	NackSender* const nack_sender_;
	std::map<uint16_t, NackInfo, DescendingSeqNumComp<uint16_t>> nack_list_;
	std::set<uint16_t, DescendingSeqNumComp<uint16_t>> keyframe_list_;
	std::set<uint16_t, DescendingSeqNumComp<uint16_t>> recovered_list_;

	// Adds a delay before send nack on packet received.
	const int64_t send_nack_delay_ms_;

	bool initialized_;
	int64_t rtt_ms_;
	uint16_t newest_seq_num_;
};
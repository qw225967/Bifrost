#include "transport_feedback_adapter.h"
#include <assert.h>
constexpr TimeDelta kSendTimeHistoryWindow = TimeDelta::Seconds(600);

InFlightBytesTracker::InFlightBytesTracker() {}
InFlightBytesTracker::~InFlightBytesTracker() {}

void InFlightBytesTracker::AddInFlightPacketBytes(
	const PacketFeedback& packet) {
	assert(packet.sent.send_time.IsFinite());
	in_flight_data_ += packet.sent.size;
}

void InFlightBytesTracker::RemoveInFlightPacketBytes(const PacketFeedback& packet) {
	if (packet.sent.send_time.IsInfinite())
		return;
	in_flight_data_ -= packet.sent.size;
}

DataSize InFlightBytesTracker::GetOutstandingData() const {
	return in_flight_data_;
}

TransportFeedbackAdapter::TransportFeedbackAdapter() = default;

//history_写
void TransportFeedbackAdapter::AddPacket(const VideoPacketSendInfo& packet_info, 
	size_t overhead_bytes, Timestamp creation_time) {	
	PacketFeedback packet;
	packet.creation_time = creation_time;
	packet.sent.sequence_number = packet_info.sequence_number;
	packet.sent.size = DataSize::Bytes(static_cast<int64_t>(packet_info.length + overhead_bytes));
	packet.sent.pacing_info = packet_info.pacing_info;

	//如果map中的packets超过了时间限制（60s),则进行清理
	std::lock_guard lock(mutex_);
	while (!history_.empty() && creation_time - history_.begin()->second.creation_time >
		kSendTimeHistoryWindow) {
		//没有进行确认的，为什么这里要减去in flight bytes?????????????????
		if (history_.begin()->second.sent.sequence_number > static_cast<int64_t>(last_ack_seq_num_)) {
			in_flight_.RemoveInFlightPacketBytes(history_.begin()->second);
		}
		history_.erase(history_.begin());
	}
	history_.insert(std::make_pair(packet.sent.sequence_number, packet));
}

//included in feedback指的是该packet是否应该包含在构件TransportFeedback的feedback中
//发送的过程中会根据 info 中的 included_in_feedback 标记确定该包是否要包含到feedback的统计计算中。
//如果没有包含则会计算为pending_untracked_size_

//history_ read
std::optional<SentPacket> TransportFeedbackAdapter::ProcessSentPacket(
	uint64_t sequence_number, Timestamp sent_time) {
	assert(sent_time.IsFinite());		
	auto iter = history_.find(sequence_number);	
	if (iter != history_.end()) {
		bool packet_retransmit = iter->second.sent.send_time.IsFinite();
		iter->second.sent.send_time = sent_time;
		last_send_time_ = std::max(last_send_time_, sent_time);
		if (!pending_untracked_size_.IsZero()) {
			if (sent_time < last_untracked_send_time_) {
				std::cout << "appending acknowledged data for out of order packet.";
			}
			iter->second.sent.prior_unacked_data += pending_untracked_size_;
			pending_untracked_size_ = DataSize::Zero();
		}
		if (!packet_retransmit) {
			if (iter->second.sent.sequence_number > static_cast<int64_t>(last_ack_seq_num_)) {
				in_flight_.AddInFlightPacketBytes(iter->second);
			}
			iter->second.sent.data_in_flight = in_flight_.GetOutstandingData();
			return iter->second.sent;
		}
		else {
			return std::nullopt;
		}
	}
	else {
		return std::nullopt;
	}	
}

std::vector<PacketResult> TransportFeedbackAdapter::ProcessTransportFeedbackInner(
	std::map<uint16_t, __uint_least64_t>& feedbacks, Timestamp feedback_receive_time) {
	//https://www.cnblogs.com/ishen/p/15249678.html 关于current-offset的设置问题参考这里 
	current_offset_ = feedback_receive_time;
	
	std::vector<PacketResult> packet_result_vector;
	packet_result_vector.reserve(feedbacks.size());

	size_t failed_lookups = 0;
	//size_t ignored = 0;
	//TimeDelta packet_offset = TimeDelta::Zero();
	
	for (const auto& packet : feedbacks) {
		int64_t seq_num = packet.first;		
		if (seq_num > last_ack_seq_num_) {
			// Starts at history_.begin() if last_ack_seq_num_ < 0, since any valid
			// sequence number is >= 0.
			for (auto iter = history_.upper_bound(last_ack_seq_num_); 
			iter != history_.upper_bound(seq_num); ++iter) {
				in_flight_.RemoveInFlightPacketBytes(iter->second);
			}
			last_ack_seq_num_ = seq_num;
		}
		auto iter = history_.find(seq_num);
		if (iter == history_.end()) {
			++failed_lookups;
			continue;
		}
		PacketFeedback packet_feedback = iter->second;
		//这里都是接收到的 packets
		packet_feedback.receive_time = Timestamp::Millis(static_cast<int64_t>(packet.second));
		history_.erase(iter);

		PacketResult result;
		result.sent_packet = packet_feedback.sent;
		result.receive_time = packet_feedback.receive_time;
		packet_result_vector.push_back(result);
	}
	if (failed_lookups > 0) {
		std::cout << "Failed to lookup send time for " << failed_lookups
			<< " packet" << (failed_lookups > 1 ? "s" : "")
			<< ". Send time history too small?";
	}
	return packet_result_vector;
}

std::optional<TransportPacketsFeedback> TransportFeedbackAdapter::ProcessTransportFeedback(
	std::map<uint16_t, uint64_t>& feedback, Timestamp feedback_receive_time) {
	if (feedback.size() == 0) {
		return std::nullopt;
	}
	TransportPacketsFeedback msg;
	msg.feedback_time = feedback_receive_time;
	msg.prior_in_flight = in_flight_.GetOutstandingData();	
	msg.packet_feedbacks = ProcessTransportFeedbackInner(feedback, feedback_receive_time);
	if (msg.packet_feedbacks.empty()) {
		return std::nullopt;
	}
	auto it = history_.find(last_ack_seq_num_);
	if (it != history_.end()) {
		msg.first_unacked_send_time = it->second.sent.send_time;
	}
	msg.data_in_flight = in_flight_.GetOutstandingData();
	return msg;
}

#pragma once

//RtpSenderEgress AddPacketToTransportFeedback中调用
//transport_feedback_observer_->OnAddPacket(packet_info);
//而RtpTransportControllerSend就是一个transport feedback observer
//好的情况是，在UdpSocket->Send之前，要包含一个结构，这个结构包含了数据的指针和size，并包含了该packet的其他信息，
//编译OnSentPacket的真正调用，知道具体确实是什么时候发送的
//另外队列只是针对rtp才排队的
//此sent packet非彼sent packet

#include "ns3/ack_header.h"
#include <map>
#include <mutex>
#include <optional>
#include "ns3/timestamp.h"
#include "ns3/common.h"

struct PacketFeedback {
	PacketFeedback() = default;
	// Time corresponding to when this object was created.
	Timestamp creation_time = Timestamp::MinusInfinity();
    SentPacket sent;
	/*Time corresponding to when the packet was received.Timestamped with the
	// receiver's clock. For unreceived packet, Timestamp::PlusInfinity() is
	// used.*/
	Timestamp receive_time = Timestamp::PlusInfinity();
};

class InFlightBytesTracker {
public:	
	InFlightBytesTracker();
	~InFlightBytesTracker();
	void AddInFlightPacketBytes(const PacketFeedback& packet);
	void RemoveInFlightPacketBytes(const PacketFeedback& packet);
	DataSize GetOutstandingData() const;
	DataSize in_flight_data_ = DataSize::Zero();
};

class TransportFeedbackAdapter {
public:
	TransportFeedbackAdapter();
	void AddPacket(const VideoPacketSendInfo& packet_info, size_t overhead_bytes, Timestamp creation_time);
	std::optional<SentPacket> ProcessSentPacket(uint64_t sequence_number, Timestamp sent_time);
	std::optional<TransportPacketsFeedback> ProcessTransportFeedback(std::map<uint16_t, uint64_t>& ack_elements, 
		Timestamp feedback_receive_time);
	DataSize GetOutstandingData() const;

private:
	enum class SendTimeHistoryStatus{ kNotAdded, kOk, kDuplicate };
	std::vector<PacketResult> ProcessTransportFeedbackInner(std::map<uint16_t, uint64_t>& feedback, 
		Timestamp feedback_receive_time);

	DataSize pending_untracked_size_ = DataSize::Zero();
	Timestamp last_send_time_ = Timestamp::MinusInfinity();
	Timestamp last_untracked_send_time_ = Timestamp::MinusInfinity();
	std::map<uint64_t, PacketFeedback> history_;

	int64_t last_ack_seq_num_ = -1;
	InFlightBytesTracker in_flight_;

	Timestamp current_offset_ = Timestamp::MinusInfinity();
	TimeDelta last_timestamp_ = TimeDelta::MinusInfinity();

	std::mutex mutex_;
};

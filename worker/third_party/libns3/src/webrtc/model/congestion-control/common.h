#pragma once
//#include "packet/ack_packet.h"
#include "ns3/timestamp.h"
#include "ns3/data_size.h"
#include "ns3/data_rate.h"
#include "ns3/packet.h"
#include <optional>
#include <vector>

static const int64_t kBitrateWindowMs = 1000;

using namespace rtc;
//每个probe packet对应的信息
struct PacedPacketInfo {
	PacedPacketInfo();
	PacedPacketInfo(int probe_cluster_id,
		int probe_cluster_min_probes,
		int probe_cluster_min_bytes);

	bool operator==(const PacedPacketInfo& rhs) const;

	// TODO(srte): Move probing info to a separate, optional struct.
	static constexpr int kNotAProbe = -1;
	int send_bitrate_bps = -1;
	int probe_cluster_id = kNotAProbe;
	int probe_cluster_min_probes = -1;
	int probe_cluster_min_bytes = -1;
	int probe_cluster_bytes_sent = 0;
};

struct SentPacket {
	Timestamp send_time = Timestamp::PlusInfinity();
	// Size of packet with overhead up to IP layer.
	DataSize size = DataSize::Zero();
	// Size of preceeding packets that are not part of feedback.
	DataSize prior_unacked_data = DataSize::Zero();
	// Probe cluster id and parameters including bitrate, number of packets and
	// number of bytes.
	PacedPacketInfo pacing_info;
	// True if the packet is an audio packet, false for video, padding, RTX etc.
	bool audio = false;
	// Transport independent sequence number, any tracked packet should have a
	// sequence number that is unique over the whole call and increasing by 1 for
	// each packet.
	int64_t sequence_number;
	// Tracked data in flight when the packet was sent, excluding unacked data.
	DataSize data_in_flight = DataSize::Zero();
};

struct PacketResult {		
	SentPacket sent_packet;
	Timestamp receive_time = Timestamp::PlusInfinity();	
	class ReceiveTimeOrder {
	public:
		bool operator()(const PacketResult& lhs, const PacketResult& rhs);
	};

	inline bool IsReceived() const { return !receive_time.IsPlusInfinity(); }
};

enum class BandwidthUsage {
	kBwNormal = 0,
	kBwUnderusing = 1,
	kBwOverusing = 2,
	kLast
};

struct TransportPacketsFeedback {
	TransportPacketsFeedback();
	TransportPacketsFeedback(const TransportPacketsFeedback& other);
	~TransportPacketsFeedback();

	Timestamp feedback_time = Timestamp::PlusInfinity();
	Timestamp first_unacked_send_time = Timestamp::PlusInfinity();
	DataSize data_in_flight = DataSize::Zero();
	DataSize prior_in_flight = DataSize::Zero();
	std::vector<PacketResult> packet_feedbacks;

	std::vector<PacketResult> ReceivedWithSendInfo() const;
	std::vector<PacketResult> LostWithSendInfo() const;
	std::vector<PacketResult> PacketsWithFeedback() const;
	std::vector<PacketResult> SortedByReceiveTime() const;
};

struct RateControlInput {
	RateControlInput(BandwidthUsage bw_state,
		const std::optional<RtcDataRate>& estimated_throughput);
	~RateControlInput();

	BandwidthUsage bw_state;
	std::optional<RtcDataRate> estimated_throughput;
};

struct TargetTransferRate {
	Timestamp at_time = Timestamp::PlusInfinity();
	// The estimate on which the target rate is based on.	
	RtcDataRate target_rate = RtcDataRate::Zero();
	RtcDataRate stable_target_rate = RtcDataRate::Zero();
	double cwnd_reduce_ratio = 0;
};

struct PacerConfig {
	Timestamp at_time = Timestamp::PlusInfinity();
	// Pacer should send at most data_window data over time_window duration.
	DataSize data_window = DataSize::Infinity();
	TimeDelta time_window = TimeDelta::PlusInfinity();
	// Pacer should send at least pad_window data over time_window duration.
	DataSize pad_window = DataSize::Zero();
	RtcDataRate data_rate() const { return data_window / time_window; }
	RtcDataRate pad_rate() const { return pad_window / time_window; }
};

struct ProbeClusterConfig {
	Timestamp at_time = Timestamp::PlusInfinity();
	RtcDataRate target_data_rate = RtcDataRate::Zero();
	TimeDelta target_duration = TimeDelta::Zero();
	int32_t target_probe_count = 0;
	int32_t id = 0;
};

// Contains updates of network controller comand state. Using optionals to
// indicate whether a member has been updated. The array of probe clusters
// should be used to send out probes if not empty.
struct NetworkControlUpdate {
	NetworkControlUpdate();
	NetworkControlUpdate(const NetworkControlUpdate&);
	~NetworkControlUpdate();
	std::optional<DataSize> congestion_window;
	std::optional<PacerConfig> pacer_config;
	std::vector<ProbeClusterConfig> probe_cluster_configs;
	std::optional<TargetTransferRate> target_rate;
};

// Process control
struct ProcessInterval {
	ProcessInterval();
	ProcessInterval(const ProcessInterval&);
	~ProcessInterval();
	Timestamp at_time = Timestamp::PlusInfinity();
	std::optional<DataSize> pacer_queue;
};

struct RoundTripTimeUpdate {
	Timestamp receive_time = Timestamp::PlusInfinity();
	TimeDelta round_trip_time = TimeDelta::PlusInfinity();
	bool smoothed = false;
};



struct ReceivedPacket {
	Timestamp send_time = Timestamp::MinusInfinity();
	Timestamp receive_time = Timestamp::PlusInfinity();
	DataSize size = DataSize::Zero();
};

struct TransportLossReport {
	Timestamp receive_time = Timestamp::PlusInfinity();
	Timestamp start_time = Timestamp::PlusInfinity();
	Timestamp end_time = Timestamp::PlusInfinity();
	uint64_t packets_lost_delta = 0;
	uint64_t packets_received_delta = 0;
};

class NetworkControllerInterface {
public:
	virtual ~NetworkControllerInterface() = default;
	virtual NetworkControlUpdate OnProcessInterval(ProcessInterval) = 0;
	virtual NetworkControlUpdate OnRoundTripTimeUpdate(
		RoundTripTimeUpdate) = 0;
	virtual NetworkControlUpdate OnSentPacket(
		SentPacket) = 0;
	virtual NetworkControlUpdate OnReceivedPacket(
		ReceivedPacket) = 0;	
	virtual NetworkControlUpdate OnTransportPacketsFeedback(
		TransportPacketsFeedback) = 0;
	
};



namespace congestion_controller {
	int GetMinBitrateBps();
	RtcDataRate GetMinBitrate();
}  // namespace congestion_controller

struct TargetRateConstraints {
	TargetRateConstraints();
	TargetRateConstraints(const TargetRateConstraints&);
	~TargetRateConstraints();
	Timestamp at_time = Timestamp::PlusInfinity();
	std::optional<RtcDataRate> min_data_rate;
	std::optional<RtcDataRate> max_data_rate;
	// The initial bandwidth estimate to base target rate on. This should be used
	// as the basis for initial OnTargetTransferRate and OnPacerConfig callbacks.
	std::optional<RtcDataRate> starting_rate;
};

struct CongestionWindowConfig {	
	std::optional<int> queue_size_ms;
	std::optional<int> min_bitrate_bps;
	std::optional<DataSize> initial_data_window;
	bool drop_frame_only = false;
};

class RateControlSettings final {
public:
	RateControlSettings();
	~RateControlSettings();
	RateControlSettings(RateControlSettings&&);	

	// When CongestionWindowPushback is enabled, the pacer is oblivious to
	// the congestion window. The relation between outstanding data and
	// the congestion window affects encoder allocations directly.
	bool UseCongestionWindow() const;
	int64_t GetCongestionWindowAdditionalTimeMs() const;
	bool UseCongestionWindowPushback() const;
	bool UseCongestionWindowDropFrameOnly() const;
	uint32_t CongestionWindowMinPushbackTargetBitrateBps() const;
	std::optional<DataSize> CongestionWindowInitialDataWindow() const;

	std::optional<double> GetPacingFactor() const;
	bool UseAlrProbing() const;	
private:	
	std::optional<double> pacing_factor;
	bool alr_probing = false;
	CongestionWindowConfig congestion_window_config_;	
};

// NOTE! `kNumMediaTypes` must be kept in sync with RtpPacketMediaType!
/* static constexpr size_t kNumMediaTypes = 5;
enum class PacketType : size_t {
  kAudio,                         // Audio media packets.
  kVideo,                         // Video media packets.
  kRetransmission,                // Retransmisions, sent as response to NACK.
  kForwardErrorCorrection,        // FEC packets.
  kPadding = kNumMediaTypes - 1,  // RTX or plain padding sent to maintain BWE.
  // Again, don't forget to udate `kNumMediaTypes` if you add another value!
}; */

static constexpr size_t kNumMediaTypes = 5;
enum PacketType : uint8_t {
	kVideo,
	kForwardErrorCorrection,
	kAck,
	kNack,
	kEchoRequest,
	kEchoReply,
	kRetransmission, 
	kPadding,
	kStop = kNumMediaTypes - 1,	
};

struct VideoPacketSendInfo {
public:
	VideoPacketSendInfo() = default;
	uint64_t sequence_number = 0;
	size_t length = 0;
	PacedPacketInfo pacing_info;
};

struct PacketToSend {
	PacketType packet_type;
	uint64_t seq; //for packet_type==kVideo
	uint32_t payload_size;
	ns3::Ptr<ns3::Packet> packet;
	size_t padding_size() { return 0; }
	size_t PayloadSize() { return payload_size; }
	PacketToSend() {}
	size_t headers_size() {
		/* if (packet_type == PacketType::kVideo || packet_type == PacketType::kRetransmission)
			return 23; 
		if (packet_type == PacketType::kForwardErrorCorrection) 
			return 6; */
			return packet->GetSize() - payload_size;
	}
};
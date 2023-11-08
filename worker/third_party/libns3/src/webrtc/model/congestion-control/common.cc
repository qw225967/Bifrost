#include "common.h"
#include <optional>
bool PacketResult::ReceiveTimeOrder::operator()(const PacketResult& lhs, const PacketResult& rhs) {
	if (lhs.receive_time != rhs.receive_time) {
		return lhs.receive_time < rhs.receive_time;
	}
	if (lhs.sent_packet.send_time != rhs.sent_packet.send_time) {
		return lhs.sent_packet.send_time < rhs.sent_packet.send_time;
	}
	return lhs.sent_packet.sequence_number < rhs.sent_packet.sequence_number;
}

PacedPacketInfo::PacedPacketInfo() = default;

PacedPacketInfo::PacedPacketInfo(int probe_cluster_id,
	int probe_cluster_min_probes,
	int probe_cluster_min_bytes)
	: probe_cluster_id(probe_cluster_id),
	probe_cluster_min_probes(probe_cluster_min_probes),
	probe_cluster_min_bytes(probe_cluster_min_bytes) {}

TransportPacketsFeedback::TransportPacketsFeedback() = default;
TransportPacketsFeedback::TransportPacketsFeedback(
	const TransportPacketsFeedback& other) = default;
TransportPacketsFeedback::~TransportPacketsFeedback() = default;

std::vector<PacketResult> TransportPacketsFeedback::ReceivedWithSendInfo()
const {
	std::vector<PacketResult> res;
	for (const PacketResult& fb : packet_feedbacks) {
		if (fb.IsReceived()) {
			res.push_back(fb);
		}
	}
	return res;
}

std::vector<PacketResult> TransportPacketsFeedback::LostWithSendInfo() const {
	std::vector<PacketResult> res;
	for (const PacketResult& fb : packet_feedbacks) {
		if (!fb.IsReceived()) {
			res.push_back(fb);
		}
	}
	return res;
}

std::vector<PacketResult> TransportPacketsFeedback::PacketsWithFeedback()
const {
	return packet_feedbacks;
}

std::vector<PacketResult> TransportPacketsFeedback::SortedByReceiveTime()
const {
	std::vector<PacketResult> res;
	for (const PacketResult& fb : packet_feedbacks) {
		if (fb.IsReceived()) {
			res.push_back(fb);
		}
	}
	std::sort(res.begin(), res.end(), PacketResult::ReceiveTimeOrder());
	return res;
}

RateControlInput::RateControlInput( 
	BandwidthUsage bw_state,
	const std::optional<RtcDataRate>& estimated_throughput)
	: bw_state(bw_state), estimated_throughput(estimated_throughput) {}

RateControlInput::~RateControlInput() = default;

namespace congestion_controller {
	int GetMinBitrateBps() {
		constexpr int kMinBitrateBps = 5000;
		return kMinBitrateBps;
	}

	RtcDataRate GetMinBitrate() {
		return RtcDataRate::BitsPerSec(GetMinBitrateBps());
	}
}

NetworkControlUpdate::NetworkControlUpdate() = default;
NetworkControlUpdate::NetworkControlUpdate(const NetworkControlUpdate&) =
default;
NetworkControlUpdate::~NetworkControlUpdate() = default;

const int kDefaultAcceptedQueueMs = 350;

const int kDefaultMinPushbackTargetBitrateBps = 30000;

RateControlSettings::RateControlSettings() = default;
RateControlSettings::~RateControlSettings() = default;
RateControlSettings::RateControlSettings(RateControlSettings&&) = default;


bool RateControlSettings::UseCongestionWindow() const {
	return static_cast<bool>(congestion_window_config_.queue_size_ms);
}

int64_t RateControlSettings::GetCongestionWindowAdditionalTimeMs() const {
	return congestion_window_config_.queue_size_ms.value_or(
		kDefaultAcceptedQueueMs);
}

bool RateControlSettings::UseCongestionWindowPushback() const {
	return congestion_window_config_.queue_size_ms &&
		congestion_window_config_.min_bitrate_bps;
}

bool RateControlSettings::UseCongestionWindowDropFrameOnly() const {
	return congestion_window_config_.drop_frame_only;
}

uint32_t RateControlSettings::CongestionWindowMinPushbackTargetBitrateBps()
const {
	return congestion_window_config_.min_bitrate_bps.value_or(
		kDefaultMinPushbackTargetBitrateBps);
}

std::optional<DataSize>
RateControlSettings::CongestionWindowInitialDataWindow() const {
	return congestion_window_config_.initial_data_window;
}

std::optional<double> RateControlSettings::GetPacingFactor() const {
	return pacing_factor;
}

bool RateControlSettings::UseAlrProbing() const {
	return alr_probing;
}

TargetRateConstraints::TargetRateConstraints() = default;
TargetRateConstraints::TargetRateConstraints(const TargetRateConstraints&) =
default;
TargetRateConstraints::~TargetRateConstraints() = default;

ProcessInterval::ProcessInterval() = default;
ProcessInterval::ProcessInterval(const ProcessInterval&) = default;
ProcessInterval::~ProcessInterval() = default;
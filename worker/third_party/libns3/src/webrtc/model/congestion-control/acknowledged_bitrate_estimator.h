#pragma once
#include <memory>
#include <vector>
#include "bitrate_estimator.h"
#include "common.h"
#include "ns3/data_rate.h"
using namespace rtc;
class AcknowledgedBitrateEstimator {
public:
	AcknowledgedBitrateEstimator();
	~AcknowledgedBitrateEstimator();

	void IncomingPacketFeedbackVector(
		const std::vector<PacketResult>& packet_feedback_vector);
	std::optional<RtcDataRate> bitrate() const;
	std::optional<RtcDataRate> PeekRate() const;
	void SetAlr(bool in_alr);
	void SetAlrEndedTime(Timestamp alr_ended_time);

private:
	std::optional<Timestamp> alr_ended_time_;
	bool in_alr_;
	std::unique_ptr<BitrateEstimator> bitrate_estimator_;
};
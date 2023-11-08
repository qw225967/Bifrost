#pragma once
#include "common.h"
#include <vector>
#include <optional>
#include <memory>
#include "aimd_rate_controller.h"
#include "trendline_estimator.h"
#include "inter_arrival_delta.h"
#include "ns3/data_rate.h"
using namespace rtc;
class DelayBasedBwe {
public:
	struct Result {
		Result();
		~Result() = default;
		bool updated;
		bool probe;
		RtcDataRate target_bitrate = RtcDataRate::Zero();
		bool recovered_from_overuse;
		bool backoff_in_alr;
	};

	DelayBasedBwe();
	~DelayBasedBwe();

	Result IncomingPacketFeedbackVector(const TransportPacketsFeedback& msg, 
		std::optional<RtcDataRate> acked_bitrate,
		std::optional<RtcDataRate> probe_bitrate,
		bool in_alr);

	void OnRttUpdate(TimeDelta avg_rtt);
	bool LatestEstimate(RtcDataRate* bitrate) const;
	void SetStartBitrate(RtcDataRate start_bitrate);
	void SetMinBitrate(RtcDataRate min_bitrate);

	TimeDelta GetExpectedBwePeriod() const;
	void SetAlrLimitedBackoffExperiment(bool enabled);
	RtcDataRate TriggerOveruse(Timestamp at_time, std::optional<RtcDataRate> link_capacity);
	RtcDataRate last_estimate() const { return prev_bitrate_; }

private:
	void IncomingPacketFeedback(const PacketResult& packet_feedback, Timestamp at_time);
	Result MaybeUpdateEstimate(
		std::optional<RtcDataRate> acked_bitrate,
		std::optional<RtcDataRate> probe_bitrate,
		bool recovered_from_overuse,
		bool in_alr,
		Timestamp at_time);

	// Updates the current remote rate estimate and returns true if a valid
	// estimate exists.
	bool UpdateEstimate(Timestamp at_time,
		std::optional<RtcDataRate> acked_bitrate,
		RtcDataRate* target_rate);	
	
	RtcDataRate prev_bitrate_;
	Timestamp last_seen_packet_;
	BandwidthUsage prev_state_;
	bool has_once_detected_overuse_;	
	bool alr_limited_backoff_enabled_;

	std::unique_ptr<InterArrivalDelta> inter_arrival_delta_;
	std::unique_ptr<TrendlineEstimator> trendline_estimator_;
	AimdRateController rate_controller_;	
};
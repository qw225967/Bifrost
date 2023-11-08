#pragma once
#include "common.h"
#include "ns3/data_rate.h"
#include "ns3/time_delta.h"
#include "link_capacity_estimator.h"

using namespace rtc;
class AimdRateController {
public:
	AimdRateController();
	~AimdRateController();

	// Returns true if the target bitrate has been initialized. This happens
    // either if it has been explicitly set via SetStartBitrate/SetEstimate, or if
    // we have measured a throughput.
	bool ValidEstimate() const;
	void SetStartBitrate(RtcDataRate start_bitrate);
	void SetMinBitrate(RtcDataRate min_bitrate);
	TimeDelta GetFeedbackInterval() const;

	// Returns true if the bitrate estimate hasn't been changed for more than
    // an RTT, or if the estimated_throughput is less than half of the current
    // estimate. Should be used to decide if we should reduce the rate further
    // when over-using.
	bool TimeToReduceFurther(Timestamp at_time,
		RtcDataRate estimated_throughput) const;

	// As above. To be used if overusing before we have measured a throughput.
	bool InitialTimeToReduceFurther(Timestamp at_time) const;

	RtcDataRate LatestEstimate() const;
	void SetRtt(TimeDelta rtt);
	RtcDataRate Update(const RateControlInput* input, Timestamp at_time);

	void SetInApplicationLimitedRegion(bool in_alr);
	void SetEstimate(RtcDataRate bitrate, Timestamp at_time);
	
	// Returns the increase rate when used bandwidth is near the link capacity.
	double GetNearMaxIncreaseRateBpsPerSecond() const;
	// Returns the expected time between overuse signals (assuming steady state).
	TimeDelta GetExpectedBandwidthPeriod() const;

private:
	enum class RateControlState{ kRcHold, kRcIncrease, kRcDecrease };
	// Update the target bitrate based on, among other things, the current rate
	// control state, the current target bitrate and the estimated throughput.
	// When in the "increase" state the bitrate will be increased either
	// additively or multiplicatively depending on the rate control region. When
	// in the "decrease" state the bitrate will be decreased to slightly below the
	// current throughput. When in the "hold" state the bitrate will be kept
	// constant to allow built up queues to drain.
	void ChangeBitrate(const RateControlInput& input, Timestamp at_time);
	RtcDataRate ClampBitrate(RtcDataRate new_bitrate) const;
	RtcDataRate MultiplicativeRateIncrease(Timestamp at_time, Timestamp last_ms, RtcDataRate current_bitrate) const;
	RtcDataRate AdditiveRateIncrease(Timestamp at_time, Timestamp last_time) const;
	void UpdateChangePeriod(Timestamp at_time);
	void ChangeState(const RateControlInput& input, Timestamp at_time);

	RtcDataRate min_configured_bitrate_;
	RtcDataRate max_configured_bitrate_;
	RtcDataRate current_bitrate_;
	RtcDataRate latest_estimated_throughput_;

	LinkCapacityEstimator link_capacity_;

	RateControlState rate_control_state_;
	Timestamp time_last_bitrate_change_;
	Timestamp time_last_bitrate_decrease_;
	Timestamp time_first_throughput_estimate_;
	bool bitrate_is_initialized_;
	double beta_;
	bool in_alr_;
	TimeDelta rtt_;

	// Allow the delay based estimate to only increase as long as application
	// limited region (alr) is not detected.
	const bool no_bitrate_increase_in_alr_;
	// Use estimated link capacity lower bound if it is higher than the
	// acknowledged rate when backing off due to overuse.
	const bool estimate_bounded_backoff_;
	// Use estimated link capacity upper bound as upper limit for increasing
	// bitrate over the acknowledged rate.
	const bool estimate_bounded_increase_;
	std::optional<RtcDataRate> last_decrease_;
};
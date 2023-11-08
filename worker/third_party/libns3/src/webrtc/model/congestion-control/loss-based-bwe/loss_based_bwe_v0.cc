/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "loss_based_bwe_v0.h"

#include <algorithm>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>

constexpr TimeDelta kBweIncreaseInterval = TimeDelta::Millis(1000);
constexpr TimeDelta kBweDecreaseInterval = TimeDelta::Millis(300);
constexpr TimeDelta kStartPhase = TimeDelta::Millis(2000);
constexpr TimeDelta kBweConverganceTime = TimeDelta::Millis(20000);
constexpr int kLimitNumPackets = 20;
constexpr RtcDataRate kDefaultMaxBitrate = RtcDataRate::BitsPerSec(1000000000);
constexpr TimeDelta kLowBitrateLogPeriod = TimeDelta::Millis(10000);
constexpr TimeDelta kRtcEventLogPeriod = TimeDelta::Millis(5000);
// Expecting that RTCP feedback is sent uniformly within [0.5, 1.5]s intervals.
constexpr TimeDelta kMaxRtcpFeedbackInterval = TimeDelta::Millis(5000);

constexpr float kDefaultLowLossThreshold = 0.02f;
constexpr float kDefaultHighLossThreshold = 0.1f;
constexpr RtcDataRate kDefaultBitrateThreshold = RtcDataRate::Zero();

struct UmaRampUpMetric {
	const char* metric_name;
	int bitrate_kbps;
};

const UmaRampUpMetric kUmaRampupMetrics[] = {
	{"WebRTC.BWE.RampUpTimeTo500kbpsInMs", 500},
	{"WebRTC.BWE.RampUpTimeTo1000kbpsInMs", 1000},
	{"WebRTC.BWE.RampUpTimeTo2000kbpsInMs", 2000} };
const size_t kNumUmaRampupMetrics =
sizeof(kUmaRampupMetrics) / sizeof(kUmaRampupMetrics[0]);

const char kBweLosExperiment[] = "WebRTC-BweLossExperiment";


bool ReadBweLossExperimentParameters(float* low_loss_threshold,
	float* high_loss_threshold,
	uint32_t* bitrate_threshold_kbps) {
	assert(low_loss_threshold);
	assert(high_loss_threshold);
	assert(bitrate_threshold_kbps);
	
	*low_loss_threshold = kDefaultLowLossThreshold;
	*high_loss_threshold = kDefaultHighLossThreshold;
	*bitrate_threshold_kbps = kDefaultBitrateThreshold.kbps();
	return false;
}


LinkCapacityTracker::LinkCapacityTracker()
	: tracking_rate(TimeDelta::Seconds(10)) {	
}

LinkCapacityTracker::~LinkCapacityTracker() {}

void LinkCapacityTracker::UpdateDelayBasedEstimate(
	Timestamp at_time,
	RtcDataRate delay_based_bitrate) {
	if (delay_based_bitrate < last_delay_based_estimate_) {
		capacity_estimate_bps_ =
			std::min(capacity_estimate_bps_, delay_based_bitrate.bps<double>());
		last_link_capacity_update_ = at_time;
	}
	last_delay_based_estimate_ = delay_based_bitrate;
}

void LinkCapacityTracker::OnStartingRate(RtcDataRate start_rate) {
	if (last_link_capacity_update_.IsInfinite())
		capacity_estimate_bps_ = start_rate.bps<double>();
}

void LinkCapacityTracker::OnRateUpdate(std::optional<RtcDataRate> acknowledged,
	RtcDataRate target,
	Timestamp at_time) {
	if (!acknowledged)
		return;
	RtcDataRate acknowledged_target = std::min(*acknowledged, target);
	if (acknowledged_target.bps() > capacity_estimate_bps_) {
		TimeDelta delta = at_time - last_link_capacity_update_;
		double alpha = delta.IsFinite() ? exp(-(delta / tracking_rate)) : 0;
		capacity_estimate_bps_ = alpha * capacity_estimate_bps_ +
			(1 - alpha) * acknowledged_target.bps<double>();
	}
	last_link_capacity_update_ = at_time;
}

void LinkCapacityTracker::OnRttBackoff(RtcDataRate backoff_rate,
	Timestamp at_time) {
	capacity_estimate_bps_ =
		std::min(capacity_estimate_bps_, backoff_rate.bps<double>());
	last_link_capacity_update_ = at_time;
}

RtcDataRate LinkCapacityTracker::estimate() const {
	return RtcDataRate::BitsPerSec(capacity_estimate_bps_);
}

RttBasedBackoff::RttBasedBackoff()
	: disabled_(false),
	configured_limit_(TimeDelta::Seconds(3)), //limit是key,后面的是缺省值
	drop_fraction_(0.8),
	drop_interval_(TimeDelta::Seconds(1)),
	bandwidth_floor_(RtcDataRate::KilobitsPerSec(5)),
	rtt_limit_(TimeDelta::PlusInfinity()),
	// By initializing this to plus infinity, we make sure that we never
	// trigger rtt backoff unless packet feedback is enabled.
	last_propagation_rtt_update_(Timestamp::PlusInfinity()),
	last_propagation_rtt_(TimeDelta::Zero()),
	last_packet_sent_(Timestamp::MinusInfinity()) {
	
}

void RttBasedBackoff::UpdatePropagationRtt(Timestamp at_time,
	TimeDelta propagation_rtt) {
	last_propagation_rtt_update_ = at_time;
	last_propagation_rtt_ = propagation_rtt;
}

TimeDelta RttBasedBackoff::CorrectedRtt(Timestamp at_time) const {
	TimeDelta time_since_rtt = at_time - last_propagation_rtt_update_;
	TimeDelta timeout_correction = time_since_rtt;
	// Avoid timeout when no packets are being sent.
	TimeDelta time_since_packet_sent = at_time - last_packet_sent_;
	timeout_correction =
		std::max(time_since_rtt - time_since_packet_sent, TimeDelta::Zero());
	return timeout_correction + last_propagation_rtt_;
}

RttBasedBackoff::~RttBasedBackoff() = default;

LossBasedBweV0::LossBasedBweV0()
	: rtt_backoff_(),
	lost_packets_since_last_loss_update_(0),
	expected_packets_since_last_loss_update_(0),
	current_target_(RtcDataRate::Zero()),
	last_logged_target_(RtcDataRate::Zero()),
	min_bitrate_configured_(
		RtcDataRate::BitsPerSec(congestion_controller::GetMinBitrateBps())), //5k是最小了
	max_bitrate_configured_(kDefaultMaxBitrate),
	last_low_bitrate_log_(Timestamp::MinusInfinity()),
	has_decreased_since_last_fraction_loss_(false),
	last_loss_feedback_(Timestamp::MinusInfinity()),
	last_loss_packet_report_(Timestamp::MinusInfinity()),
	last_fraction_loss_(0),
	last_logged_fraction_loss_(0),
	last_round_trip_time_(TimeDelta::Zero()),
	receiver_limit_(RtcDataRate::PlusInfinity()),
	delay_based_limit_(RtcDataRate::PlusInfinity()),
	time_last_decrease_(Timestamp::MinusInfinity()),
	first_report_time_(Timestamp::MinusInfinity()),
	initially_lost_packets_(0),
	bitrate_at_2_seconds_(RtcDataRate::Zero()),
	uma_update_state_(kNoUpdate),
	uma_rtt_state_(kNoUpdate),
	rampup_uma_stats_updated_(kNumUmaRampupMetrics, false),	
	low_loss_threshold_(kDefaultLowLossThreshold),
	high_loss_threshold_(kDefaultHighLossThreshold),
	bitrate_threshold_(kDefaultBitrateThreshold),	
	disable_receiver_limit_caps_only_(false) {

	uint32_t bitrate_threshold_kbps;
	if (ReadBweLossExperimentParameters(&low_loss_threshold_,
		&high_loss_threshold_,
		&bitrate_threshold_kbps));
	bitrate_threshold_ = RtcDataRate::KilobitsPerSec(bitrate_threshold_kbps);
}

LossBasedBweV0::~LossBasedBweV0() {}

void LossBasedBweV0::Reset() {
	lost_packets_since_last_loss_update_ = 0;
	expected_packets_since_last_loss_update_ = 0;
	current_target_ = RtcDataRate::Zero();
	min_bitrate_configured_ =
		RtcDataRate::BitsPerSec(congestion_controller::GetMinBitrateBps());
	max_bitrate_configured_ = kDefaultMaxBitrate;
	last_low_bitrate_log_ = Timestamp::MinusInfinity();
	has_decreased_since_last_fraction_loss_ = false;
	last_loss_feedback_ = Timestamp::MinusInfinity();
	last_loss_packet_report_ = Timestamp::MinusInfinity();
	last_fraction_loss_ = 0;
	last_logged_fraction_loss_ = 0;
	last_round_trip_time_ = TimeDelta::Zero();
	receiver_limit_ = RtcDataRate::PlusInfinity();
	delay_based_limit_ = RtcDataRate::PlusInfinity();
	time_last_decrease_ = Timestamp::MinusInfinity();
	first_report_time_ = Timestamp::MinusInfinity();
	initially_lost_packets_ = 0;
	bitrate_at_2_seconds_ = RtcDataRate::Zero();
	uma_update_state_ = kNoUpdate;
	uma_rtt_state_ = kNoUpdate;	
}

void LossBasedBweV0::SetBitrates(
	std::optional<RtcDataRate> send_bitrate,
	RtcDataRate min_bitrate,
	RtcDataRate max_bitrate,
	Timestamp at_time) {
	SetMinMaxBitrate(min_bitrate, max_bitrate);
	if (send_bitrate) {
		link_capacity_.OnStartingRate(*send_bitrate);
		SetSendBitrate(*send_bitrate, at_time);
	}
}

void LossBasedBweV0::SetSendBitrate(RtcDataRate bitrate,
	Timestamp at_time) {
	assert(bitrate > RtcDataRate::Zero());
	// Reset to avoid being capped by the estimate.
	delay_based_limit_ = RtcDataRate::PlusInfinity();
	UpdateTargetBitrate(bitrate, at_time);
	// Clear last sent bitrate history so the new value can be used directly
	// and not capped.
	min_bitrate_history_.clear();
}

void LossBasedBweV0::SetMinMaxBitrate(RtcDataRate min_bitrate,
	RtcDataRate max_bitrate) {
	min_bitrate_configured_ =
		std::max(min_bitrate, congestion_controller::GetMinBitrate());
	if (max_bitrate > RtcDataRate::Zero() && max_bitrate.IsFinite()) {
		max_bitrate_configured_ = std::max(min_bitrate_configured_, max_bitrate);
	}
	else {
		max_bitrate_configured_ = kDefaultMaxBitrate;
	}
}

int LossBasedBweV0::GetMinBitrate() const {
	return min_bitrate_configured_.bps<int>();
}

RtcDataRate LossBasedBweV0::target_rate() const {
	RtcDataRate target = current_target_;
	if (!disable_receiver_limit_caps_only_)
		target = std::min(target, receiver_limit_);
	return std::max(min_bitrate_configured_, target);
}

RtcDataRate LossBasedBweV0::GetEstimatedLinkCapacity() const {
	return link_capacity_.estimate();
}

void LossBasedBweV0::UpdateReceiverEstimate(Timestamp at_time,
	RtcDataRate bandwidth) {
	// TODO(srte): Ensure caller passes PlusInfinity, not zero, to represent no
	// limitation.
	receiver_limit_ = bandwidth.IsZero() ? RtcDataRate::PlusInfinity() : bandwidth;
	ApplyTargetLimits(at_time);
}

void LossBasedBweV0::UpdateDelayBasedEstimate(Timestamp at_time,
	RtcDataRate bitrate) {
	link_capacity_.UpdateDelayBasedEstimate(at_time, bitrate);
	// TODO(srte): Ensure caller passes PlusInfinity, not zero, to represent no
	// limitation.
	delay_based_limit_ = bitrate.IsZero() ? RtcDataRate::PlusInfinity() : bitrate;
	ApplyTargetLimits(at_time);
}

void LossBasedBweV0::SetAcknowledgedRate(
	std::optional<RtcDataRate> acknowledged_rate,
	Timestamp at_time) {
	acknowledged_rate_ = acknowledged_rate;
	if (!acknowledged_rate.has_value()) {
		return;
	}	
}

void LossBasedBweV0::UpdatePacketsLost(int64_t packets_lost,
	int64_t number_of_packets,
	Timestamp at_time) {
	last_loss_feedback_ = at_time;
	if (first_report_time_.IsInfinite())
		first_report_time_ = at_time;

	// Check sequence number diff and weight loss report
	if (number_of_packets > 0) {
		int64_t expected =
			expected_packets_since_last_loss_update_ + number_of_packets;

		// Don't generate a loss rate until it can be based on enough packets.
		if (expected < kLimitNumPackets) {
			// Accumulate reports.
			expected_packets_since_last_loss_update_ = expected;
			lost_packets_since_last_loss_update_ += packets_lost;
			return;
		}

		has_decreased_since_last_fraction_loss_ = false;
		int64_t lost_q8 = (lost_packets_since_last_loss_update_ + packets_lost)
			<< 8;
		last_fraction_loss_ = std::min<int>(lost_q8 / expected, 255);

		// Reset accumulators.
		lost_packets_since_last_loss_update_ = 0;
		expected_packets_since_last_loss_update_ = 0;
		last_loss_packet_report_ = at_time;
		UpdateEstimate(at_time);
	}
	UpdateUmaStatsPacketsLost(at_time, packets_lost);
}

void LossBasedBweV0::UpdateUmaStatsPacketsLost(Timestamp at_time,
	int packets_lost) {
	RtcDataRate bitrate_kbps =
		RtcDataRate::KilobitsPerSec((current_target_.bps() + 500) / 1000);
	for (size_t i = 0; i < kNumUmaRampupMetrics; ++i) {
		if (!rampup_uma_stats_updated_[i] &&
			bitrate_kbps.kbps() >= kUmaRampupMetrics[i].bitrate_kbps) {			
			rampup_uma_stats_updated_[i] = true;
		}
	}
	if (IsInStartPhase(at_time)) {
		initially_lost_packets_ += packets_lost;
	}
	else if (uma_update_state_ == kNoUpdate) {
		uma_update_state_ = kFirstDone;
		bitrate_at_2_seconds_ = bitrate_kbps;		
	}
	else if (uma_update_state_ == kFirstDone &&
		at_time - first_report_time_ >= kBweConverganceTime) {
		uma_update_state_ = kDone;
		/* int bitrate_diff_kbps = std::max(
			bitrate_at_2_seconds_.kbps<int>() - bitrate_kbps.kbps<int>(), 0);*/
	}
}

void LossBasedBweV0::UpdateRtt(TimeDelta rtt, Timestamp at_time) {
	// Update RTT if we were able to compute an RTT based on this RTCP.
	// FlexFEC doesn't send RTCP SR, which means we won't be able to compute RTT.
	if (rtt > TimeDelta::Zero())
		last_round_trip_time_ = rtt;

	if (!IsInStartPhase(at_time) && uma_rtt_state_ == kNoUpdate) {
		uma_rtt_state_ = kDone;		
	}
}

void LossBasedBweV0::UpdateEstimate(Timestamp at_time) {
	if (rtt_backoff_.CorrectedRtt(at_time) > rtt_backoff_.rtt_limit_) {
		if (at_time - time_last_decrease_ >= rtt_backoff_.drop_interval_ &&
			current_target_ > rtt_backoff_.bandwidth_floor_) {
			time_last_decrease_ = at_time;
			RtcDataRate new_bitrate =
				std::max(current_target_ * rtt_backoff_.drop_fraction_,
					rtt_backoff_.bandwidth_floor_);
			link_capacity_.OnRttBackoff(new_bitrate, at_time);
			UpdateTargetBitrate(new_bitrate, at_time);
			return;
		}
		// TODO(srte): This is likely redundant in most cases.
		ApplyTargetLimits(at_time);
		return;
	}

	// We trust the REMB and/or delay-based estimate during the first 2 seconds if
	// we haven't had any packet loss reported, to allow startup bitrate probing.
	if (last_fraction_loss_ == 0 && IsInStartPhase(at_time)) {
		RtcDataRate new_bitrate = current_target_;
		// TODO(srte): We should not allow the new_bitrate to be larger than the
		// receiver limit here.
		if (receiver_limit_.IsFinite())
			new_bitrate = std::max(receiver_limit_, new_bitrate);
		if (delay_based_limit_.IsFinite())
			new_bitrate = std::max(delay_based_limit_, new_bitrate);		

		if (new_bitrate != current_target_) {
			min_bitrate_history_.clear();
			min_bitrate_history_.push_back(
				std::make_pair(at_time, current_target_));

			UpdateTargetBitrate(new_bitrate, at_time);
			return;
		}
	}
	UpdateMinHistory(at_time);
	if (last_loss_packet_report_.IsInfinite()) {
		// No feedback received.
		// TODO(srte): This is likely redundant in most cases.
		ApplyTargetLimits(at_time);
		return;
	}	

	TimeDelta time_since_loss_packet_report = at_time - last_loss_packet_report_;
	if (time_since_loss_packet_report < 1.2 * kMaxRtcpFeedbackInterval) {
		// We only care about loss above a given bitrate threshold.
		float loss = last_fraction_loss_ / 256.0f;
		// We only make decisions based on loss when the bitrate is above a
		// threshold. This is a crude way of handling loss which is uncorrelated
		// to congestion.
		if (current_target_ < bitrate_threshold_ || loss <= low_loss_threshold_) {
			// Loss < 2%: Increase rate by 8% of the min bitrate in the last
			// kBweIncreaseInterval.
			// Note that by remembering the bitrate over the last second one can
			// rampup up one second faster than if only allowed to start ramping
			// at 8% per second rate now. E.g.:
			//   If sending a constant 100kbps it can rampup immediately to 108kbps
			//   whenever a receiver report is received with lower packet loss.
			//   If instead one would do: current_bitrate_ *= 1.08^(delta time),
			//   it would take over one second since the lower packet loss to achieve
			//   108kbps.
			RtcDataRate new_bitrate = RtcDataRate::BitsPerSec(
				min_bitrate_history_.front().second.bps() * 1.08 + 0.5);

			// Add 1 kbps extra, just to make sure that we do not get stuck
			// (gives a little extra increase at low rates, negligible at higher
			// rates).
			new_bitrate += RtcDataRate::BitsPerSec(1000);
			UpdateTargetBitrate(new_bitrate, at_time);
			return;
		}
		else if (current_target_ > bitrate_threshold_) {
			if (loss <= high_loss_threshold_) {
				// Loss between 2% - 10%: Do nothing.
			}
			else {
				// Loss > 10%: Limit the rate decreases to once a kBweDecreaseInterval
				// + rtt.
				if (!has_decreased_since_last_fraction_loss_ &&
					(at_time - time_last_decrease_) >=
					(kBweDecreaseInterval + last_round_trip_time_)) {
					time_last_decrease_ = at_time;

					// Reduce rate:
					//   newRate = rate * (1 - 0.5*lossRate);
					//   where packetLoss = 256*lossRate;
					RtcDataRate new_bitrate = RtcDataRate::BitsPerSec(
						(current_target_.bps() *
							static_cast<double>(512 - last_fraction_loss_)) /
						512.0);
					has_decreased_since_last_fraction_loss_ = true;
					UpdateTargetBitrate(new_bitrate, at_time);
					return;
				}
			}
		}
	}
	// TODO(srte): This is likely redundant in most cases.
	ApplyTargetLimits(at_time);
}

void LossBasedBweV0::UpdatePropagationRtt(
	Timestamp at_time,
	TimeDelta propagation_rtt) {
	rtt_backoff_.UpdatePropagationRtt(at_time, propagation_rtt);
}

void LossBasedBweV0::OnSentPacket(const SentPacket& sent_packet) {
	// Only feedback-triggering packets will be reported here.
	rtt_backoff_.last_packet_sent_ = sent_packet.send_time;
}

bool LossBasedBweV0::IsInStartPhase(Timestamp at_time) const {
	return first_report_time_.IsInfinite() ||
		at_time - first_report_time_ < kStartPhase;
}

void LossBasedBweV0::UpdateMinHistory(Timestamp at_time) {
	// Remove old data points from history.
	// Since history precision is in ms, add one so it is able to increase
	// bitrate if it is off by as little as 0.5ms.
	while (!min_bitrate_history_.empty() &&
		at_time - min_bitrate_history_.front().first + TimeDelta::Millis(1) >
		kBweIncreaseInterval) {
		min_bitrate_history_.pop_front();
	}

	// Typical minimum sliding-window algorithm: Pop values higher than current
	// bitrate before pushing it.
	while (!min_bitrate_history_.empty() &&
		current_target_ <= min_bitrate_history_.back().second) {
		min_bitrate_history_.pop_back();
	}

	min_bitrate_history_.push_back(std::make_pair(at_time, current_target_));
}

RtcDataRate LossBasedBweV0::GetUpperLimit() const {
	RtcDataRate upper_limit = delay_based_limit_;
	if (disable_receiver_limit_caps_only_)
		upper_limit = std::min(upper_limit, receiver_limit_);
	return std::min(upper_limit, max_bitrate_configured_);
}



void LossBasedBweV0::IncomingPacketFeedbackVector(const TransportPacketsFeedback& report) {

}


void LossBasedBweV0::UpdateTargetBitrate(RtcDataRate new_bitrate,
	Timestamp at_time) {
	new_bitrate = std::min(new_bitrate, GetUpperLimit());
	if (new_bitrate < min_bitrate_configured_) {		
		new_bitrate = min_bitrate_configured_;
	}
	current_target_ = new_bitrate;	
	link_capacity_.OnRateUpdate(acknowledged_rate_, current_target_, at_time);
}

void LossBasedBweV0::ApplyTargetLimits(Timestamp at_time) {
	UpdateTargetBitrate(current_target_, at_time);
}





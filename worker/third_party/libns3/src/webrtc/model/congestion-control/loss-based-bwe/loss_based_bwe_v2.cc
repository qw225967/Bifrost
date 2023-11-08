/*
 *  Copyright 2021 The WebRTC project authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "loss_based_bwe_v2.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

#include "ns3/container.h"
#include <optional>
#include "ns3/array_view.h"

bool IsValid(RtcDataRate RtcDataRate) {
	return RtcDataRate.IsFinite();
}

bool IsValid(Timestamp timestamp) {
	return timestamp.IsFinite();
}

struct PacketResultsSummary {
	int num_packets = 0;
	int num_lost_packets = 0;
	DataSize total_size = DataSize::Zero();
	Timestamp first_send_time = Timestamp::PlusInfinity();
	Timestamp last_send_time = Timestamp::MinusInfinity();
};

// Returns a `PacketResultsSummary` where `first_send_time` is `PlusInfinity,
// and `last_send_time` is `MinusInfinity`, if `packet_results` is empty.
PacketResultsSummary GetPacketResultsSummary(
	rtc::ArrayView<const PacketResult> packet_results) {
	PacketResultsSummary packet_results_summary;

	packet_results_summary.num_packets = packet_results.size();
	for (const PacketResult& packet : packet_results) {
		if (!packet.IsReceived()) {
			packet_results_summary.num_lost_packets++;
		}
		packet_results_summary.total_size += packet.sent_packet.size;
		packet_results_summary.first_send_time = std::min(
			packet_results_summary.first_send_time, packet.sent_packet.send_time);
		packet_results_summary.last_send_time = std::max(
			packet_results_summary.last_send_time, packet.sent_packet.send_time);
	}

	return packet_results_summary;
}

double GetLossProbability(double inherent_loss,
	RtcDataRate loss_limited_bandwidth,
	RtcDataRate sending_rate) {
	if (inherent_loss < 0.0 || inherent_loss > 1.0) {
		std::cout << "The inherent loss must be in [0,1]: "
			<< inherent_loss;
		inherent_loss = std::min(std::max(inherent_loss, 0.0), 1.0);
	}
	if (!sending_rate.IsFinite()) {
		std::cout << "The sending rate must be finite";
	}
	if (!loss_limited_bandwidth.IsFinite()) {
		std::cout << "The loss limited bandwidth must be finite ";
	}

	// We approximate the loss model
	//     loss_probability = inherent_loss + (1 - inherent_loss) *
	//     max(0, sending_rate - bandwidth) / sending_rate
	// by
	//     loss_probability = inherent_loss +
	//     max(0, sending_rate - bandwidth) / sending_rate
	// as it allows for simpler calculations and makes little difference in
	// practice.
	double loss_probability = inherent_loss;
	if (IsValid(sending_rate) && IsValid(loss_limited_bandwidth) &&
		(sending_rate > loss_limited_bandwidth)) {
		loss_probability += (sending_rate - loss_limited_bandwidth) / sending_rate;
	}
	return std::min(std::max(loss_probability, 1.0e-6), 1.0 - 1.0e-6);
}



LossBasedBwe::LossBasedBwe()
	: config_(CreateConfig()) {
	if (!config_.has_value()) {
		std::cout << "The configuration does not specify that the "
			"estimator should be enabled, disabling it.";
		return;
	}
	if (!IsConfigValid()) {
		std::cout
			<< "The configuration is not valid, disabling the estimator.";
		config_.reset();
		return;
	}

	current_estimate_.inherent_loss = config_->initial_inherent_loss_estimate;
	observations_.resize(config_->observation_window_size);
	temporal_weights_.resize(config_->observation_window_size);
	instant_upper_bound_temporal_weights_.resize(
		config_->observation_window_size);
	CalculateTemporalWeights();
}

bool LossBasedBwe::IsEnabled() const {
	return true;
}

bool LossBasedBwe::IsReady() const {
	return IsEnabled() && IsValid(current_estimate_.loss_limited_bandwidth) &&
		num_observations_ > 0;
}

RtcDataRate LossBasedBwe::GetBandwidthEstimate() const {
	if (!IsReady()) {
		if (!IsEnabled()) {
			std::cout
				<< "The estimator must be enabled before it can be used.";
		}
		else {
			if (!IsValid(current_estimate_.loss_limited_bandwidth)) {
				std::cout
					<< "The estimator must be initialized before it can be used.";
			}
			if (num_observations_ <= 0) {
				std::cout << "The estimator must receive enough loss "
					"statistics before it can be used.";
			}
		}
		return RtcDataRate::PlusInfinity();
	}

	return std::min(current_estimate_.loss_limited_bandwidth,
		GetInstantUpperBound());
}

void LossBasedBwe::SetAcknowledgedBitrate(RtcDataRate acknowledged_bitrate) {
	if (IsValid(acknowledged_bitrate)) {
		acknowledged_bitrate_ = acknowledged_bitrate;
	}
	else {
		std::cout << "The acknowledged bitrate must be finite.";			
	}
}

void LossBasedBwe::SetBandwidthEstimate(RtcDataRate bandwidth_estimate) {
	if (IsValid(bandwidth_estimate)) {
		current_estimate_.loss_limited_bandwidth = bandwidth_estimate;
	}
	else {
		std::cout << "The bandwidth estimate must be finite.";
	}
}

void LossBasedBwe::UpdateBandwidthEstimate(
	rtc::ArrayView<const PacketResult> packet_results,
	RtcDataRate delay_based_estimate) {
	if (!IsEnabled()) {
		std::cout
			<< "The estimator must be enabled before it can be used.";
		return;
	}
	if (packet_results.empty()) {
		std::cout
			<< "The estimate cannot be updated without any loss statistics.";
		return;
	}

	if (!PushBackObservation(packet_results)) {
		return;
	}

	if (!IsValid(current_estimate_.loss_limited_bandwidth)) {
		std::cout
			<< "The estimator must be initialized before it can be used.";
		return;
	}

	ChannelParameters best_candidate = current_estimate_;
	double objective_max = std::numeric_limits<double>::lowest();
	for (ChannelParameters candidate : GetCandidates(delay_based_estimate)) {
		NewtonsMethodUpdate(candidate);

		const double candidate_objective = GetObjective(candidate);
		if (candidate_objective > objective_max) {
			objective_max = candidate_objective;
			best_candidate = candidate;
		}
	}
	if (best_candidate.loss_limited_bandwidth <
		current_estimate_.loss_limited_bandwidth) {
		last_time_estimate_reduced_ = last_send_time_most_recent_observation_;
	}
	current_estimate_ = best_candidate;
}

// Returns a `LossBasedBweV2::Config` iff the `key_value_config` specifies a
// configuration for the `LossBasedBweV2` which is explicitly enabled.
std::optional<LossBasedBwe::Config> LossBasedBwe::CreateConfig() {	

	std::optional<Config> config;	
	config.emplace();
	config->bandwidth_rampup_upper_bound_factor = 1.1;
	config->rampup_acceleration_max_factor = 0.0;
	config->rampup_acceleration_maxout_time = TimeDelta::Seconds(60);
	config->candidate_factors = {1.05, 1.0, 0.95};
	config->higher_bandwidth_bias_factor = 0.00001;
	config->higher_log_bandwidth_bias_factor = 0.001;
	config->inherent_loss_lower_bound = 1.0e-3;
	config->inherent_loss_upper_bound_bandwidth_balance = RtcDataRate::KilobitsPerSec(15.0);
	config->inherent_loss_upper_bound_offset = 0.05;
	config->initial_inherent_loss_estimate = 0.01;
	config->newton_iterations =1;
	config->newton_step_size = 0.5;
	config->append_acknowledged_rate_candidate = true;
	config->append_delay_based_estimate_candidate = false;
	config->observation_duration_lower_bound = TimeDelta::Seconds(1);
	config->observation_window_size = 20;
	config->sending_rate_smoothing_factor = 0.0;
	config->instant_upper_bound_temporal_weight_factor = 0.99;
	config->instant_upper_bound_bandwidth_balance = RtcDataRate::KilobitsPerSec(15.0);
	config->instant_upper_bound_loss_offset = 0.05;
	config->temporal_weight_factor = 0.99;
	return config;
}

bool LossBasedBwe::IsConfigValid() const {
	if (!config_.has_value()) {
		return false;
	}

	bool valid = true;

	if (config_->bandwidth_rampup_upper_bound_factor <= 1.0) {
		std::cout
			<< "The bandwidth rampup upper bound factor must be greater than 1: "
			<< config_->bandwidth_rampup_upper_bound_factor;
		valid = false;
	}
	if (config_->rampup_acceleration_max_factor < 0.0) {
		std::cout
			<< "The rampup acceleration max factor must be non-negative.: "
			<< config_->rampup_acceleration_max_factor;
		valid = false;
	}
	if (config_->rampup_acceleration_maxout_time <= TimeDelta::Zero()) {
		std::cout
			<< "The rampup acceleration maxout time must be above zero: "
			<< config_->rampup_acceleration_maxout_time.seconds();
		valid = false;
	}
	for (double candidate_factor : config_->candidate_factors) {
		if (candidate_factor <= 0.0) {
			std::cout << "All candidate factors must be greater than zero: "
				<< candidate_factor;
			valid = false;
		}
	}

	// Ensure that the configuration allows generation of at least one candidate
	// other than the current estimate.
	if (!config_->append_acknowledged_rate_candidate &&
		!config_->append_delay_based_estimate_candidate &&
		!c_any_of(config_->candidate_factors,
			[](double cf) { return cf != 1.0; })) {
		std::cout
			<< "The configuration does not allow generating candidates. Specify "
			"a candidate factor other than 1.0, allow the acknowledged rate "
			"to be a candidate, and/or allow the delay based estimate to be a "
			"candidate.";
		valid = false;
	}

	if (config_->higher_bandwidth_bias_factor < 0.0) {
		std::cout
			<< "The higher bandwidth bias factor must be non-negative: "
			<< config_->higher_bandwidth_bias_factor;
		valid = false;
	}
	if (config_->inherent_loss_lower_bound < 0.0 ||
		config_->inherent_loss_lower_bound >= 1.0) {
		std::cout << "The inherent loss lower bound must be in [0, 1): "
			<< config_->inherent_loss_lower_bound;
		valid = false;
	}
	if (config_->inherent_loss_upper_bound_bandwidth_balance <=
		RtcDataRate::Zero()) {
		std::cout
			<< "The inherent loss upper bound bandwidth balance "
			"must be positive.";
		valid = false;
	}
	if (config_->inherent_loss_upper_bound_offset <
		config_->inherent_loss_lower_bound ||
		config_->inherent_loss_upper_bound_offset >= 1.0) {
		std::cout << "The inherent loss upper bound must be greater "
			"than or equal to the inherent "
			"loss lower bound, which is "
			<< config_->inherent_loss_lower_bound
			<< ", and less than 1: "
			<< config_->inherent_loss_upper_bound_offset;
		valid = false;
	}
	if (config_->initial_inherent_loss_estimate < 0.0 ||
		config_->initial_inherent_loss_estimate >= 1.0) {
		std::cout
			<< "The initial inherent loss estimate must be in [0, 1): "
			<< config_->initial_inherent_loss_estimate;
		valid = false;
	}
	if (config_->newton_iterations <= 0) {
		std::cout << "The number of Newton iterations must be positive: "
			<< config_->newton_iterations;
		valid = false;
	}
	if (config_->newton_step_size <= 0.0) {
		std::cout << "The Newton step size must be positive: "
			<< config_->newton_step_size;
		valid = false;
	}
	if (config_->observation_duration_lower_bound <= TimeDelta::Zero()) {
		std::cout
			<< "The observation duration lower bound must be positive.";
		valid = false;
	}
	if (config_->observation_window_size < 2) {
		std::cout << "The observation window size must be at least 2: "
			<< config_->observation_window_size;
		valid = false;
	}
	if (config_->sending_rate_smoothing_factor < 0.0 ||
		config_->sending_rate_smoothing_factor >= 1.0) {
		std::cout
			<< "The sending rate smoothing factor must be in [0, 1): "
			<< config_->sending_rate_smoothing_factor;
		valid = false;
	}
	if (config_->instant_upper_bound_temporal_weight_factor <= 0.0 ||
		config_->instant_upper_bound_temporal_weight_factor > 1.0) {
		std::cout
			<< "The instant upper bound temporal weight factor must be in (0, 1]"
			<< config_->instant_upper_bound_temporal_weight_factor;
		valid = false;
	}
	if (config_->instant_upper_bound_bandwidth_balance <= RtcDataRate::Zero()) {
		std::cout
			<< "The instant upper bound bandwidth balance must be positive.";
		valid = false;
	}
	if (config_->instant_upper_bound_loss_offset < 0.0 ||
		config_->instant_upper_bound_loss_offset >= 1.0) {
		std::cout
			<< "The instant upper bound loss offset must be in [0, 1): "
			<< config_->instant_upper_bound_loss_offset;
		valid = false;
	}
	if (config_->temporal_weight_factor <= 0.0 ||
		config_->temporal_weight_factor > 1.0) {
		std::cout << "The temporal weight factor must be in (0, 1]: "
			<< config_->temporal_weight_factor;
		valid = false;
	}

	return valid;
}

double LossBasedBwe::GetAverageReportedLossRatio() const {
	if (num_observations_ <= 0) {
		return 0.0;
	}

	int num_packets = 0;
	int num_lost_packets = 0;
	for (const Observation& observation : observations_) {
		if (!observation.IsInitialized()) {
			continue;
		}

		double instant_temporal_weight =
			instant_upper_bound_temporal_weights_[(num_observations_ - 1) -
			observation.id];
		num_packets += instant_temporal_weight * observation.num_packets;
		num_lost_packets += instant_temporal_weight * observation.num_lost_packets;
	}

	return static_cast<double>(num_lost_packets) / num_packets;
}

RtcDataRate LossBasedBwe::GetCandidateBandwidthUpperBound() const {
	if (!acknowledged_bitrate_.has_value())
		return RtcDataRate::PlusInfinity();

	RtcDataRate candidate_bandwidth_upper_bound =
		config_->bandwidth_rampup_upper_bound_factor * (*acknowledged_bitrate_);

	if (config_->rampup_acceleration_max_factor > 0.0) {
		const TimeDelta time_since_bandwidth_reduced = std::min(
			config_->rampup_acceleration_maxout_time,
			std::max(TimeDelta::Zero(), last_send_time_most_recent_observation_ -
				last_time_estimate_reduced_));
		const double rampup_acceleration = config_->rampup_acceleration_max_factor *
			time_since_bandwidth_reduced /
			config_->rampup_acceleration_maxout_time;

		candidate_bandwidth_upper_bound +=
			rampup_acceleration * (*acknowledged_bitrate_);
	}
	return candidate_bandwidth_upper_bound;
}

std::vector<LossBasedBwe::ChannelParameters> LossBasedBwe::GetCandidates(
	RtcDataRate delay_based_estimate) const {
	std::vector<RtcDataRate> bandwidths;
	for (double candidate_factor : config_->candidate_factors) {
		bandwidths.push_back(candidate_factor *
			current_estimate_.loss_limited_bandwidth);
	}

	if (acknowledged_bitrate_.has_value() &&
		config_->append_acknowledged_rate_candidate) {
		bandwidths.push_back(*acknowledged_bitrate_);
	}

	if (IsValid(delay_based_estimate) &&
		config_->append_delay_based_estimate_candidate) {
		bandwidths.push_back(delay_based_estimate);
	}

	const RtcDataRate candidate_bandwidth_upper_bound =
		GetCandidateBandwidthUpperBound();

	std::vector<ChannelParameters> candidates;
	candidates.resize(bandwidths.size());
	for (size_t i = 0; i < bandwidths.size(); ++i) {
		ChannelParameters candidate = current_estimate_;
		candidate.loss_limited_bandwidth = std::min(
			bandwidths[i], std::max(current_estimate_.loss_limited_bandwidth,
				candidate_bandwidth_upper_bound));
		candidate.inherent_loss = GetFeasibleInherentLoss(candidate);
		candidates[i] = candidate;
	}
	return candidates;
}

LossBasedBwe::Derivatives LossBasedBwe::GetDerivatives(
	const ChannelParameters& channel_parameters) const {
	Derivatives derivatives;

	for (const Observation& observation : observations_) {
		if (!observation.IsInitialized()) {
			continue;
		}

		double loss_probability = GetLossProbability(
			channel_parameters.inherent_loss,
			channel_parameters.loss_limited_bandwidth, observation.sending_rate);

		double temporal_weight =
			temporal_weights_[(num_observations_ - 1) - observation.id];

		derivatives.first +=
			temporal_weight *
			((observation.num_lost_packets / loss_probability) -
				(observation.num_received_packets / (1.0 - loss_probability)));
		derivatives.second -=
			temporal_weight *
			((observation.num_lost_packets / std::pow(loss_probability, 2)) +
				(observation.num_received_packets /
					std::pow(1.0 - loss_probability, 2)));
	}

	if (derivatives.second >= 0.0) {
		std::cout << "The second derivative is mathematically guaranteed "
			"to be negative but is "
			<< derivatives.second << ".";
		derivatives.second = -1.0e-6;
	}

	return derivatives;
}

double LossBasedBwe::GetFeasibleInherentLoss(
	const ChannelParameters& channel_parameters) const {
	return std::min(
		std::max(channel_parameters.inherent_loss,
			config_->inherent_loss_lower_bound),
		GetInherentLossUpperBound(channel_parameters.loss_limited_bandwidth));
}

double LossBasedBwe::GetInherentLossUpperBound(RtcDataRate bandwidth) const {
	if (bandwidth.IsZero()) {
		return 1.0;
	}

	double inherent_loss_upper_bound =
		config_->inherent_loss_upper_bound_offset +
		config_->inherent_loss_upper_bound_bandwidth_balance / bandwidth;
	return std::min(inherent_loss_upper_bound, 1.0);
}

double LossBasedBwe::GetHighBandwidthBias(RtcDataRate bandwidth) const {
	if (IsValid(bandwidth)) {
		return config_->higher_bandwidth_bias_factor * bandwidth.kbps() +
			config_->higher_log_bandwidth_bias_factor *
			std::log(1.0 + bandwidth.kbps());
	}
	return 0.0;
}

double LossBasedBwe::GetObjective(
	const ChannelParameters& channel_parameters) const {
	double objective = 0.0;

	const double high_bandwidth_bias =
		GetHighBandwidthBias(channel_parameters.loss_limited_bandwidth);

	for (const Observation& observation : observations_) {
		if (!observation.IsInitialized()) {
			continue;
		}

		double loss_probability = GetLossProbability(
			channel_parameters.inherent_loss,
			channel_parameters.loss_limited_bandwidth, observation.sending_rate);

		double temporal_weight =
			temporal_weights_[(num_observations_ - 1) - observation.id];

		objective +=
			temporal_weight *
			((observation.num_lost_packets * std::log(loss_probability)) +
				(observation.num_received_packets * std::log(1.0 - loss_probability)));
		objective +=
			temporal_weight * high_bandwidth_bias * observation.num_packets;
	}

	return objective;
}

RtcDataRate LossBasedBwe::GetSendingRate(
	RtcDataRate instantaneous_sending_rate) const {
	if (num_observations_ <= 0) {
		return instantaneous_sending_rate;
	}

	const int most_recent_observation_idx =
		(num_observations_ - 1) % config_->observation_window_size;
	const Observation& most_recent_observation =
		observations_[most_recent_observation_idx];
	RtcDataRate sending_rate_previous_observation =
		most_recent_observation.sending_rate;

	return config_->sending_rate_smoothing_factor *
		sending_rate_previous_observation +
		(1.0 - config_->sending_rate_smoothing_factor) *
		instantaneous_sending_rate;
}

RtcDataRate LossBasedBwe::GetInstantUpperBound() const {
	return cached_instant_upper_bound_.value_or(RtcDataRate::PlusInfinity());
}

void LossBasedBwe::CalculateInstantUpperBound() {
	RtcDataRate instant_limit = RtcDataRate::PlusInfinity();
	const double average_reported_loss_ratio = GetAverageReportedLossRatio();
	if (average_reported_loss_ratio > config_->instant_upper_bound_loss_offset) {
		instant_limit = config_->instant_upper_bound_bandwidth_balance /
			(average_reported_loss_ratio -
				config_->instant_upper_bound_loss_offset);
	}
	cached_instant_upper_bound_ = instant_limit;
}

void LossBasedBwe::CalculateTemporalWeights() {
	for (int i = 0; i < config_->observation_window_size; ++i) {
		temporal_weights_[i] = std::pow(config_->temporal_weight_factor, i);
		instant_upper_bound_temporal_weights_[i] =
			std::pow(config_->instant_upper_bound_temporal_weight_factor, i);
	}
}

void LossBasedBwe::NewtonsMethodUpdate(
	ChannelParameters& channel_parameters) const {
	if (num_observations_ <= 0) {
		return;
	}

	for (int i = 0; i < config_->newton_iterations; ++i) {
		const Derivatives derivatives = GetDerivatives(channel_parameters);
		channel_parameters.inherent_loss -=
			config_->newton_step_size * derivatives.first / derivatives.second;
		channel_parameters.inherent_loss =
			GetFeasibleInherentLoss(channel_parameters);
	}
}

bool LossBasedBwe::PushBackObservation(
	rtc::ArrayView<const PacketResult> packet_results) {
	if (packet_results.empty()) {
		return false;
	}

	PacketResultsSummary packet_results_summary =
		GetPacketResultsSummary(packet_results);

	partial_observation_.num_packets += packet_results_summary.num_packets;
	partial_observation_.num_lost_packets +=
		packet_results_summary.num_lost_packets;
	partial_observation_.size += packet_results_summary.total_size;

	// This is the first packet report we have received.
	if (!IsValid(last_send_time_most_recent_observation_)) {
		last_send_time_most_recent_observation_ =
			packet_results_summary.first_send_time;
	}

	const Timestamp last_send_time = packet_results_summary.last_send_time;
	const TimeDelta observation_duration =
		last_send_time - last_send_time_most_recent_observation_;

	// Too small to be meaningful.
	if (observation_duration < config_->observation_duration_lower_bound) {
		return false;
	}

	last_send_time_most_recent_observation_ = last_send_time;

	Observation observation;
	observation.num_packets = partial_observation_.num_packets;
	observation.num_lost_packets = partial_observation_.num_lost_packets;
	observation.num_received_packets =
		observation.num_packets - observation.num_lost_packets;
	observation.sending_rate =
		GetSendingRate(partial_observation_.size / observation_duration);
	observation.id = num_observations_++;
	observations_[observation.id % config_->observation_window_size] =
		observation;

	partial_observation_ = PartialObservation();

	CalculateInstantUpperBound();
	return true;
}



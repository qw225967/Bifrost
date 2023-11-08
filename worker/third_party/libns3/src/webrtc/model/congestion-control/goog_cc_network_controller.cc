/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "goog_cc_network_controller.h"

#include <inttypes.h>
#include <stdio.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>


#include "ns3/time_delta.h"


 // From RTCPSender video report interval.
constexpr TimeDelta kLossUpdateInterval = TimeDelta::Millis(1000);

// Pacing-rate relative to our target send rate.
// Multiplicative factor that is applied to the target bitrate to calculate
// the number of bytes that can be transmitted per interval.
// Increasing this factor will result in lower delays in cases of bitrate
// overshoots from the encoder.
constexpr float kDefaultPaceMultiplier = 2.5f;

// If the probe result is far below the current throughput estimate
// it's unlikely that the probe is accurate, so we don't want to drop too far.
// However, if we actually are overusing, we want to drop to something slightly
// below the current throughput estimate to drain the network queues.
constexpr double kProbeDropThroughputFraction = 0.85;

int64_t GetBpsOrDefault(const std::optional<RtcDataRate>& rate,
	int64_t fallback_bps) {
	if (rate && rate->IsFinite()) {
		return rate->bps();
	}
	else {
		return fallback_bps;
	}
}	


GoogCcNetworkController::GoogCcNetworkController(TargetRateConstraints constraints)
	: packet_feedback_only_(true),	
	use_min_allocatable_as_lower_bound_(false),
	ignore_probes_lower_than_network_estimate_(false),
	limit_probes_lower_than_throughput_estimate_(false),
	loss_based_stable_rate_(false),	//什么意思？包括以上几个参数应该如何的来进行配置呢？
	congestion_window_pushback_controller_(nullptr),	
	delay_based_bwe_(new DelayBasedBwe()),
	last_loss_based_target_rate_(*constraints.starting_rate),
	last_pushback_target_rate_(last_loss_based_target_rate_),
	last_stable_target_rate_(last_loss_based_target_rate_),	
	min_total_allocated_bitrate_(RtcDataRate::Zero()),
	max_total_allocated_bitrate_(RtcDataRate::Zero()),
	max_padding_rate_(RtcDataRate::Zero())
{
	if (delay_based_bwe_) {
		delay_based_bwe_->SetMinBitrate(congestion_controller::GetMinBitrate());
	}

	//loss based
	//loss_based_bwe_ = std::make_unique<LossBasedBweV0>();
	loss_based_bwe_ = nullptr;
	
	//congestion_window_pushback_controller_ = std::make_unique<CongestionWindowPushbackController>();

	AlrDetectorConfig alr_detector_config;
	alr_detector_ = std::make_unique<AlrDetector>(alr_detector_config);

	//probe
	probe_controller_ = nullptr;

	//probe_bitrate_estimator_ = std::make_unique<ProbeBitrateEstimator>();
	//probe_controller_ = std::make_unique<ProbeController>();
	acknowledged_bitrate_estimator_ = std::make_unique<AcknowledgedBitrateEstimator>();
}

GoogCcNetworkController::~GoogCcNetworkController() {}



NetworkControlUpdate GoogCcNetworkController::OnProcessInterval(
	ProcessInterval msg) {
	NetworkControlUpdate update;
	
	if (congestion_window_pushback_controller_ && msg.pacer_queue) {
		congestion_window_pushback_controller_->UpdatePacingQueue(
			msg.pacer_queue->bytes());
	}
	MaybeTriggerOnNetworkChanged(&update, msg.at_time);
	return update;
}


NetworkControlUpdate GoogCcNetworkController::OnRoundTripTimeUpdate(
	RoundTripTimeUpdate msg) {
	if (packet_feedback_only_ || msg.smoothed)
		return NetworkControlUpdate();
	assert(!msg.round_trip_time.IsZero());
	if (loss_based_bwe_) {
		loss_based_bwe_->UpdateRtt(msg.round_trip_time, msg.receive_time);
	}
	if (delay_based_bwe_) {
		delay_based_bwe_->OnRttUpdate(msg.round_trip_time);
	}

	return NetworkControlUpdate();
}

//在webrtc中，webrtc_video_engine.cc的OnPacketSent中调用call->OnSentPacket
//在BaseChannel的SignalSentPacket_n中调用media_channel()->OnPacketSent
//在rtp transport的SignalSentPacket中调用
/*
* void RtpTransport::OnSentPacket(PacketTransportInternal* packet_transport,
                                const SentPacket& sent_packet) {
  RTC_DCHECK(packet_transport == rtp_packet_transport_ ||
             packet_transport == rtcp_packet_transport_);
  SignalSentPacket(sent_packet);
}
这个时间应该是发送到队列的时候还是实际发送出去的时候呢？
*/
NetworkControlUpdate GoogCcNetworkController::OnSentPacket(
	SentPacket sent_packet) {
	
	acknowledged_bitrate_estimator_->SetAlr(
		alr_detector_->GetApplicationLimitedRegionStartTime().has_value());

	if (!first_packet_sent_) {
		first_packet_sent_ = true;
		// Initialize feedback time to send time to allow estimation of RTT until
		// first feedback is received.
		if (loss_based_bwe_) {
			loss_based_bwe_->UpdatePropagationRtt(sent_packet.send_time,
				TimeDelta::Zero());
		}
	}
	if (loss_based_bwe_) {
		loss_based_bwe_->OnSentPacket(sent_packet);
	}

	if (congestion_window_pushback_controller_) {
		congestion_window_pushback_controller_->UpdateOutstandingData(
			sent_packet.data_in_flight.bytes());
		NetworkControlUpdate update;
		MaybeTriggerOnNetworkChanged(&update, sent_packet.send_time);
		return update;
	}
	else {
		return NetworkControlUpdate();
	}
}

NetworkControlUpdate GoogCcNetworkController::OnReceivedPacket(
	ReceivedPacket received_packet) {
	last_packet_received_time_ = received_packet.receive_time;
	return NetworkControlUpdate();
}

void GoogCcNetworkController::UpdateCongestionWindowSize() {
	TimeDelta min_feedback_max_rtt = TimeDelta::Millis(
		*std::min_element(feedback_max_rtts_.begin(), feedback_max_rtts_.end()));

	const DataSize kMinCwnd = DataSize::Bytes(2 * 1500);
	TimeDelta time_window =
		min_feedback_max_rtt +
		TimeDelta::Millis(
			rate_control_settings_.GetCongestionWindowAdditionalTimeMs());

	DataSize data_window = last_loss_based_target_rate_ * time_window;
	if (current_data_window_) {
		data_window =
			std::max(kMinCwnd, (data_window + current_data_window_.value()) / 2);
	}
	else {
		data_window = std::max(kMinCwnd, data_window);
	}
	current_data_window_ = data_window;
}

//要自己来构建这个TransportPacketsFeedback
NetworkControlUpdate GoogCcNetworkController::OnTransportPacketsFeedback(
	TransportPacketsFeedback report) {
	if (report.packet_feedbacks.empty()) {
		// TODO(bugs.webrtc.org/10125): Design a better mechanism to safe-guard
		// against building very large network queues.
		return NetworkControlUpdate();
	}

	if (congestion_window_pushback_controller_) {
		congestion_window_pushback_controller_->UpdateOutstandingData(
			report.data_in_flight.bytes());
	}
	TimeDelta max_feedback_rtt = TimeDelta::MinusInfinity();
	TimeDelta min_propagation_rtt = TimeDelta::PlusInfinity();
	Timestamp max_recv_time = Timestamp::MinusInfinity();

	std::vector<PacketResult> feedbacks = report.ReceivedWithSendInfo();
	for (const auto& feedback : feedbacks)
		max_recv_time = std::max(max_recv_time, feedback.receive_time);

	for (const auto& feedback : feedbacks) {
		TimeDelta feedback_rtt =
			report.feedback_time - feedback.sent_packet.send_time;
		TimeDelta min_pending_time = feedback.receive_time - max_recv_time;
		TimeDelta propagation_rtt = feedback_rtt - min_pending_time;
		max_feedback_rtt = std::max(max_feedback_rtt, feedback_rtt);
		min_propagation_rtt = std::min(min_propagation_rtt, propagation_rtt);
	}

	if (max_feedback_rtt.IsFinite()) {
		feedback_max_rtts_.push_back(max_feedback_rtt.ms());
		const size_t kMaxFeedbackRttWindow = 32;
		if (feedback_max_rtts_.size() > kMaxFeedbackRttWindow)
			feedback_max_rtts_.pop_front();
		// TODO(srte): Use time since last unacknowledged packet.
		if (loss_based_bwe_) {
			loss_based_bwe_->UpdatePropagationRtt(report.feedback_time,
				min_propagation_rtt);
		}
	}
	//这里的意思是不是只有packet feedback，而没有loss report?
	if (packet_feedback_only_) {
		if (!feedback_max_rtts_.empty()) {
			int64_t sum_rtt_ms = std::accumulate(feedback_max_rtts_.begin(),
				feedback_max_rtts_.end(), 0);
			int64_t mean_rtt_ms = sum_rtt_ms / feedback_max_rtts_.size();
			if (delay_based_bwe_) {
				delay_based_bwe_->OnRttUpdate(TimeDelta::Millis(mean_rtt_ms));
			}
		}

		TimeDelta feedback_min_rtt = TimeDelta::PlusInfinity();
		for (const auto& packet_feedback : feedbacks) {
			TimeDelta pending_time = packet_feedback.receive_time - max_recv_time;
			TimeDelta rtt = report.feedback_time -
				packet_feedback.sent_packet.send_time - pending_time;
			// Value used for predicting NACK round trip time in FEC controller.
			feedback_min_rtt = std::min(rtt, feedback_min_rtt);
		}
		if (feedback_min_rtt.IsFinite() && loss_based_bwe_) {
			loss_based_bwe_->UpdateRtt(feedback_min_rtt, report.feedback_time);
		}

		expected_packets_since_last_loss_update_ +=
			report.PacketsWithFeedback().size();
		for (const auto& packet_feedback : report.PacketsWithFeedback()) {
			if (!packet_feedback.IsReceived())
				lost_packets_since_last_loss_update_ += 1;
		}
		if (report.feedback_time > next_loss_update_) {
			next_loss_update_ = report.feedback_time + kLossUpdateInterval;
			if (loss_based_bwe_) {
				loss_based_bwe_->UpdatePacketsLost(
					lost_packets_since_last_loss_update_,
					expected_packets_since_last_loss_update_, report.feedback_time);
			}
			expected_packets_since_last_loss_update_ = 0;
			lost_packets_since_last_loss_update_ = 0;
		}
	}
	std::optional<int64_t> alr_start_time =
		alr_detector_->GetApplicationLimitedRegionStartTime();

	if (previously_in_alr_ && !alr_start_time.has_value()) {
		int64_t now_ms = report.feedback_time.ms();
		acknowledged_bitrate_estimator_->SetAlrEndedTime(report.feedback_time);
		probe_controller_->SetAlrEndedTimeMs(now_ms);
	}
	previously_in_alr_ = alr_start_time.has_value();
	acknowledged_bitrate_estimator_->IncomingPacketFeedbackVector(
		report.SortedByReceiveTime());
	auto acknowledged_bitrate = acknowledged_bitrate_estimator_->bitrate();
	if (loss_based_bwe_) {
		loss_based_bwe_->SetAcknowledgedRate(acknowledged_bitrate,
			report.feedback_time);
		loss_based_bwe_->IncomingPacketFeedbackVector(report);
	}
	for (const auto& feedback : report.SortedByReceiveTime()) {
		if (feedback.sent_packet.pacing_info.probe_cluster_id !=
			PacedPacketInfo::kNotAProbe) {
			if (probe_controller_) {
				probe_bitrate_estimator_->HandleProbeAndEstimateBitrate(feedback);
			}
		}
	}
	
	std::optional<RtcDataRate> probe_bitrate;
	if (probe_bitrate_estimator_) {
		probe_bitrate = probe_bitrate_estimator_->FetchAndResetLastEstimatedBitrate();
	}
	if (ignore_probes_lower_than_network_estimate_ && probe_bitrate &&
		 *probe_bitrate < delay_based_bwe_->last_estimate()) {
		probe_bitrate.reset();
	}
	if (limit_probes_lower_than_throughput_estimate_ && probe_bitrate &&
		acknowledged_bitrate) {
		// Limit the backoff to something slightly below the acknowledged
		// bitrate. ("Slightly below" because we want to drain the queues
		// if we are actually overusing.)
		// The acknowledged bitrate shouldn't normally be higher than the delay
		// based estimate, but it could happen e.g. due to packet bursts or
		// encoder overshoot. We use std::min to ensure that a probe result
		// below the current BWE never causes an increase.
		RtcDataRate limit =
			std::min(delay_based_bwe_->last_estimate(),
				*acknowledged_bitrate * kProbeDropThroughputFraction);
		probe_bitrate = std::max(*probe_bitrate, limit);
	}

	NetworkControlUpdate update;
	bool recovered_from_overuse = false;
	bool backoff_in_alr = false;

	DelayBasedBwe::Result result;
	result = delay_based_bwe_->IncomingPacketFeedbackVector(
		report, acknowledged_bitrate, probe_bitrate,
		alr_start_time.has_value());

	if (result.updated) {
		if (result.probe) {
			if (loss_based_bwe_) {
				loss_based_bwe_->SetSendBitrate(result.target_bitrate,
					report.feedback_time);
			}
		}
		// Since SetSendBitrate now resets the delay-based estimate, we have to
		// call UpdateDelayBasedEstimate after SetSendBitrate.
		if (loss_based_bwe_) {
			loss_based_bwe_->UpdateDelayBasedEstimate(report.feedback_time,
				result.target_bitrate);
		}
		// Update the estimate in the ProbeController, in case we want to probe.
		MaybeTriggerOnNetworkChanged(&update, report.feedback_time);
	}
	recovered_from_overuse = result.recovered_from_overuse;
	backoff_in_alr = result.backoff_in_alr;

	if (recovered_from_overuse && probe_controller_) {
		probe_controller_->SetAlrStartTimeMs(alr_start_time);
		auto probes = probe_controller_->RequestProbe(report.feedback_time.ms());
		update.probe_cluster_configs.insert(update.probe_cluster_configs.end(),
			probes.begin(), probes.end());
	}
	else if (backoff_in_alr && probe_controller_) {
		// If we just backed off during ALR, request a new probe.
		auto probes = probe_controller_->RequestProbe(report.feedback_time.ms());
		update.probe_cluster_configs.insert(update.probe_cluster_configs.end(),
			probes.begin(), probes.end());
	}

	// No valid RTT could be because send-side BWE isn't used, in which case
	// we don't try to limit the outstanding packets.
	if (rate_control_settings_.UseCongestionWindow() &&
		max_feedback_rtt.IsFinite()) {
		UpdateCongestionWindowSize();
	}
	if (congestion_window_pushback_controller_ && current_data_window_) {
		congestion_window_pushback_controller_->SetDataWindow(
			*current_data_window_);
	}
	else {
		update.congestion_window = current_data_window_;
	}

	return update;
}

PacerConfig GoogCcNetworkController::GetPacingRates(Timestamp at_time) const {
	// Pacing rate is based on target rate before congestion window pushback,
	// because we don't want to build queues in the pacer when pushback occurs.
	RtcDataRate pacing_rate =
		std::max(min_total_allocated_bitrate_, last_loss_based_target_rate_) *
		pacing_factor_;
	RtcDataRate padding_rate =
		std::min(max_padding_rate_, last_pushback_target_rate_);
	PacerConfig msg;
	msg.at_time = at_time;
	msg.time_window = TimeDelta::Seconds(1);
	msg.data_window = pacing_rate * msg.time_window;
	msg.pad_window = padding_rate * msg.time_window;
	return msg;
}

NetworkControlUpdate GoogCcNetworkController::GetNetworkState(
	Timestamp at_time) const {
	NetworkControlUpdate update;
	update.target_rate = TargetTransferRate();	

	update.target_rate->at_time = at_time;
	update.target_rate->target_rate = last_pushback_target_rate_;
	update.target_rate->stable_target_rate =
		loss_based_bwe_->GetEstimatedLinkCapacity();
	update.pacer_config = GetPacingRates(at_time);
	update.congestion_window = current_data_window_;
	return update;
}

void GoogCcNetworkController::MaybeTriggerOnNetworkChanged(
	NetworkControlUpdate* update,
	Timestamp at_time) {
	uint8_t fraction_loss = 0;	
	TimeDelta round_trip_time = TimeDelta::Zero();	
	RtcDataRate loss_based_target_rate = RtcDataRate::Zero();
	if (loss_based_bwe_) {
		fraction_loss = loss_based_bwe_->fraction_loss();
		round_trip_time = loss_based_bwe_->round_trip_time();
		loss_based_target_rate = loss_based_bwe_->target_rate();
	}
	RtcDataRate pushback_target_rate = loss_based_target_rate;	

	double cwnd_reduce_ratio = 0.0;
	if (congestion_window_pushback_controller_) {
		int64_t pushback_rate =
			congestion_window_pushback_controller_->UpdateTargetBitrate(
				loss_based_target_rate.bps());
		pushback_rate = std::max<int64_t>(loss_based_bwe_->GetMinBitrate(),
			pushback_rate);
		pushback_target_rate = RtcDataRate::BitsPerSec(pushback_rate);
		if (rate_control_settings_.UseCongestionWindowDropFrameOnly()) {
			cwnd_reduce_ratio = static_cast<double>(loss_based_target_rate.bps() -
				pushback_target_rate.bps()) /
				loss_based_target_rate.bps();
		}
	}
	RtcDataRate stable_target_rate = RtcDataRate::Zero();
	if (loss_based_bwe_) {
		loss_based_bwe_->GetEstimatedLinkCapacity();
	}
	if (loss_based_stable_rate_) {
		stable_target_rate = std::min(stable_target_rate, loss_based_target_rate);
	}
	else {
		stable_target_rate = std::min(stable_target_rate, pushback_target_rate);
	}

	if ((loss_based_target_rate != last_loss_based_target_rate_) ||
		(fraction_loss != last_estimated_fraction_loss_) ||
		(round_trip_time != last_estimated_round_trip_time_) ||
		(pushback_target_rate != last_pushback_target_rate_) ||
		(stable_target_rate != last_stable_target_rate_)) {
		last_loss_based_target_rate_ = loss_based_target_rate;
		last_pushback_target_rate_ = pushback_target_rate;
		last_estimated_fraction_loss_ = fraction_loss;
		last_estimated_round_trip_time_ = round_trip_time;
		last_stable_target_rate_ = stable_target_rate;

		if (loss_based_target_rate != RtcDataRate::Zero()) {
			alr_detector_->SetEstimatedBitrate(loss_based_target_rate.bps());
		}

		//TimeDelta bwe_period = delay_based_bwe_->GetExpectedBwePeriod();

		TargetTransferRate target_rate_msg;
		target_rate_msg.at_time = at_time;
		if (rate_control_settings_.UseCongestionWindowDropFrameOnly()) {
			target_rate_msg.target_rate = loss_based_target_rate;
			target_rate_msg.cwnd_reduce_ratio = cwnd_reduce_ratio;
		}
		else {
			target_rate_msg.target_rate = pushback_target_rate;
		}
		target_rate_msg.stable_target_rate = stable_target_rate;
		

		update->target_rate = target_rate_msg;
		if (loss_based_target_rate != RtcDataRate::Zero() && probe_controller_) {
			auto probes = probe_controller_->SetEstimatedBitrate(
				loss_based_target_rate.bps(), at_time.ms());
			update->probe_cluster_configs.insert(update->probe_cluster_configs.end(),
				probes.begin(), probes.end());
			update->pacer_config = GetPacingRates(at_time);

			std::cout << "bwe " << at_time.ms() << " pushback_target_bps="
				<< last_pushback_target_rate_.bps()
				<< " estimate_bps=" << loss_based_target_rate.bps();
		}
	}
}




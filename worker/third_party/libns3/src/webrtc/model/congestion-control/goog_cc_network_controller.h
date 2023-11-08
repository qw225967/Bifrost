#pragma once

#include <stdint.h>

#include <deque>
#include <memory>
#include <vector>

#include "common.h"
#include "delay_based_bwe.h"
#include "acknowledged_bitrate_estimator.h"
#include "congestion_window_pushback_controller.h"
#include "ns3/probe_controller.h"
#include "ns3/probe_bitrate_estimator.h"
#include "ns3/alr_detector.h"
#include "ns3/loss_based_bwe_v0.h"
#include "ns3/data_rate.h"
using namespace rtc;
class GoogCcNetworkController : public NetworkControllerInterface {
public:
	GoogCcNetworkController(TargetRateConstraints constraints);

	GoogCcNetworkController() = delete;
	GoogCcNetworkController(const GoogCcNetworkController&) = delete;
	GoogCcNetworkController& operator=(const GoogCcNetworkController&) = delete;

	~GoogCcNetworkController() override;

	// NetworkControllerInterface
	
	NetworkControlUpdate OnProcessInterval(ProcessInterval msg) override;
	
	NetworkControlUpdate OnRoundTripTimeUpdate(RoundTripTimeUpdate msg) override;
	NetworkControlUpdate OnSentPacket(SentPacket msg) override;
	NetworkControlUpdate OnReceivedPacket(ReceivedPacket msg) override;	
	NetworkControlUpdate OnTransportPacketsFeedback(
		TransportPacketsFeedback msg) override;
	

	NetworkControlUpdate GetNetworkState(Timestamp at_time) const;

private:		
	void MaybeTriggerOnNetworkChanged(NetworkControlUpdate* update,
		Timestamp at_time);
	void UpdateCongestionWindowSize();
	PacerConfig GetPacingRates(Timestamp at_time) const;
	const bool packet_feedback_only_;
	/*FieldTrialFlag safe_reset_on_route_change_;
	FieldTrialFlag safe_reset_acknowledged_rate_;*/
	const bool use_min_allocatable_as_lower_bound_;
	const bool ignore_probes_lower_than_network_estimate_;
	const bool limit_probes_lower_than_throughput_estimate_;
	const RateControlSettings rate_control_settings_;
	const bool loss_based_stable_rate_;

	std::unique_ptr<ProbeController> probe_controller_;
	std::unique_ptr<CongestionWindowPushbackController>
		congestion_window_pushback_controller_;

	std::unique_ptr<LossBasedBweV0> loss_based_bwe_;
	std::unique_ptr<AlrDetector> alr_detector_;
	std::unique_ptr<ProbeBitrateEstimator> probe_bitrate_estimator_;
	std::unique_ptr<DelayBasedBwe> delay_based_bwe_;
	std::unique_ptr<AcknowledgedBitrateEstimator>	acknowledged_bitrate_estimator_;	

	RtcDataRate min_target_rate_ = RtcDataRate::Zero();
	RtcDataRate min_data_rate_ = RtcDataRate::Zero();
	RtcDataRate max_data_rate_ = RtcDataRate::PlusInfinity();
	std::optional<RtcDataRate> starting_rate_;

	bool first_packet_sent_ = false;
	

	Timestamp next_loss_update_ = Timestamp::MinusInfinity();
	int lost_packets_since_last_loss_update_ = 0;
	int expected_packets_since_last_loss_update_ = 0;

	std::deque<int64_t> feedback_max_rtts_;

	RtcDataRate last_loss_based_target_rate_;
	RtcDataRate last_pushback_target_rate_;
	RtcDataRate last_stable_target_rate_;

	std::optional<uint8_t> last_estimated_fraction_loss_ = 0;
	TimeDelta last_estimated_round_trip_time_ = TimeDelta::PlusInfinity();
	Timestamp last_packet_received_time_ = Timestamp::MinusInfinity();

	double pacing_factor_;
	RtcDataRate min_total_allocated_bitrate_;	
	RtcDataRate max_total_allocated_bitrate_;
	RtcDataRate max_padding_rate_;

	bool previously_in_alr_ = false;

	std::optional<DataSize> current_data_window_;

	/* packet_feedback_only_(true),	
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
	max_padding_rate_(RtcDataRate::Zero()) */
};



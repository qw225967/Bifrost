#pragma once
#include <cstddef>
#include <vector>
#include <optional>
#include "ns3/common.h"
#include "ns3/array_view.h"
#include "ns3/data_rate.h"
#include "ns3/data_size.h"
#include "ns3/timestamp.h"
#include "ns3/time_delta.h"

class LossBasedBwe {
	LossBasedBwe();
	LossBasedBwe(const LossBasedBwe&) = delete;
	LossBasedBwe& operator=(const LossBasedBwe&) = delete;
	~LossBasedBwe() = default;

    bool IsEnabled() const;
	// Returns true iff a BWE can be calculated, i.e., the estimator has been
	// initialized with a BWE and then has received enough `PacketResult`s.
	bool IsReady() const;

	// Returns `RtcDataRate::PlusInfinity` if no BWE can be calculated.
	RtcDataRate GetBandwidthEstimate() const;

	void SetAcknowledgedBitrate(RtcDataRate acknowledged_bitrate);
	void SetBandwidthEstimate(RtcDataRate bandwidth_estimate);
	void UpdateBandwidthEstimate(rtc::ArrayView<const PacketResult> packet_results, RtcDataRate delay_based_estimate);

private:
	struct ChannelParameters {
		double inherent_loss = 0.0;
		RtcDataRate loss_limited_bandwidth = RtcDataRate::MinusInfinity();
	};

    struct Config {
        double bandwidth_rampup_upper_bound_factor = 0.0;
        double rampup_acceleration_max_factor = 0.0;
        TimeDelta rampup_acceleration_maxout_time = TimeDelta::Zero();
        std::vector<double> candidate_factors;
        double higher_bandwidth_bias_factor = 0.0;
        double higher_log_bandwidth_bias_factor = 0.0;
        double inherent_loss_lower_bound = 0.0;
        RtcDataRate inherent_loss_upper_bound_bandwidth_balance =
            RtcDataRate::MinusInfinity();
        double inherent_loss_upper_bound_offset = 0.0;
        double initial_inherent_loss_estimate = 0.0;
        int newton_iterations = 0;
        double newton_step_size = 0.0;
        bool append_acknowledged_rate_candidate = true;
        bool append_delay_based_estimate_candidate = false;
        TimeDelta observation_duration_lower_bound = TimeDelta::Zero();
        int observation_window_size = 0;
        double sending_rate_smoothing_factor = 0.0;
        double instant_upper_bound_temporal_weight_factor = 0.0;
        RtcDataRate instant_upper_bound_bandwidth_balance = RtcDataRate::MinusInfinity();
        double instant_upper_bound_loss_offset = 0.0;
        double temporal_weight_factor = 0.0;
    };

    struct Derivatives {
        double first = 0.0;
        double second = 0.0;
    };

    struct Observation {
        bool IsInitialized() const { return id != -1; }

        int num_packets = 0;
        int num_lost_packets = 0;
        int num_received_packets = 0;
        RtcDataRate sending_rate = RtcDataRate::MinusInfinity();
        int id = -1;
    };

    struct PartialObservation {
        int num_packets = 0;
        int num_lost_packets = 0;
        DataSize size = DataSize::Zero();
    };

    static std::optional<Config> CreateConfig();
    bool IsConfigValid() const;

    // Returns `0.0` if not enough loss statistics have been received.
    double GetAverageReportedLossRatio() const;
    std::vector<ChannelParameters> GetCandidates(
        RtcDataRate delay_based_estimate) const;
    RtcDataRate GetCandidateBandwidthUpperBound() const;
    Derivatives GetDerivatives(const ChannelParameters& channel_parameters) const;
    double GetFeasibleInherentLoss(
        const ChannelParameters& channel_parameters) const;
    double GetInherentLossUpperBound(RtcDataRate bandwidth) const;
    double GetHighBandwidthBias(RtcDataRate bandwidth) const;
    double GetObjective(const ChannelParameters& channel_parameters) const;
    RtcDataRate GetSendingRate(RtcDataRate instantaneous_sending_rate) const;
    RtcDataRate GetInstantUpperBound() const;
    void CalculateInstantUpperBound();

    void CalculateTemporalWeights();
    void NewtonsMethodUpdate(ChannelParameters& channel_parameters) const;

    // Returns false if no observation was created.
    bool PushBackObservation(rtc::ArrayView<const PacketResult> packet_results);

    std::optional<RtcDataRate> acknowledged_bitrate_;
    std::optional<Config> config_;
    ChannelParameters current_estimate_;
    int num_observations_ = 0;
    std::vector<Observation> observations_;
    PartialObservation partial_observation_;
    Timestamp last_send_time_most_recent_observation_ = Timestamp::PlusInfinity();
    Timestamp last_time_estimate_reduced_ = Timestamp::MinusInfinity();
    std::optional<RtcDataRate> cached_instant_upper_bound_;
    std::vector<double> instant_upper_bound_temporal_weights_;
    std::vector<double> temporal_weights_;
};
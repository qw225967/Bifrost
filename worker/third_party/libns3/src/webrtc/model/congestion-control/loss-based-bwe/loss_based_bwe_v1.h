#pragma once
/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include <vector>
#include "ns3/data_rate.h"
#include "ns3/time_delta.h"
#include "ns3/timestamp.h"
#include "ns3/common.h"

struct LossBasedControlConfig {
	LossBasedControlConfig();
	LossBasedControlConfig(const LossBasedControlConfig&);
	LossBasedControlConfig& operator=(const LossBasedControlConfig&) = default;
	~LossBasedControlConfig();
	bool enabled;
	double min_increase_factor;
	double max_increase_factor;
	TimeDelta increase_low_rtt;
	TimeDelta increase_high_rtt;
	double decrease_factor;
	TimeDelta loss_window;
	TimeDelta loss_max_window;
	TimeDelta acknowledged_rate_max_window;
	RtcDataRate increase_offset;
	RtcDataRate loss_bandwidth_balance_increase;
	RtcDataRate loss_bandwidth_balance_decrease;
	RtcDataRate loss_bandwidth_balance_reset;
	double loss_bandwidth_balance_exponent;
	bool allow_resets;
	TimeDelta decrease_interval;
	TimeDelta loss_report_timeout;
};

// Estimates an upper BWE limit based on loss.
// It requires knowledge about lost packets and acknowledged bitrate.
// Ie, this class require transport feedback.
class LossBasedBandwidthEstimation {
public:
	explicit LossBasedBandwidthEstimation();
	// Returns the new estimate.
	RtcDataRate Update(Timestamp at_time,
		RtcDataRate min_bitrate,
		RtcDataRate wanted_bitrate,
		TimeDelta last_round_trip_time);
	void UpdateAcknowledgedBitrate(RtcDataRate acknowledged_bitrate,
		Timestamp at_time);
	void Initialize(RtcDataRate bitrate);
	bool Enabled() const { return config_.enabled; }
	// Returns true if LossBasedBandwidthEstimation is enabled and have
	// received loss statistics. Ie, this class require transport feedback.
	bool InUse() const {
		return Enabled() && last_loss_packet_report_.IsFinite();
	}
	void UpdateLossStatistics(const std::vector<PacketResult>& packet_results,
		Timestamp at_time);
	RtcDataRate GetEstimate() const { return loss_based_bitrate_; }

private:
	friend class GoogCcStatePrinter;
	void Reset(RtcDataRate bitrate);
	double loss_increase_threshold() const;
	double loss_decrease_threshold() const;
	double loss_reset_threshold() const;

	RtcDataRate decreased_bitrate() const;

	const LossBasedControlConfig config_;
	double average_loss_;
	double average_loss_max_;
	RtcDataRate loss_based_bitrate_;
	RtcDataRate acknowledged_bitrate_max_;
	Timestamp acknowledged_bitrate_last_update_;
	Timestamp time_last_decrease_;
	bool has_decreased_since_last_loss_report_;
	Timestamp last_loss_packet_report_;
	double last_loss_ratio_;
};




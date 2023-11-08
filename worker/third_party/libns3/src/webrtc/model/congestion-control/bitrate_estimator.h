#pragma once

// Computes a bayesian estimate of the throughput given acks containing
// the arrival time and payload size. Samples which are far from the current
// estimate or are based on few packets are given a smaller weight, as they
// are considered to be more likely to have been caused by, e.g., delay spikes
// unrelated to congestion.
#include "ns3/data_size.h"
#include "ns3/data_rate.h"
#include "ns3/timestamp.h"
#include <optional>

using namespace rtc;
class BitrateEstimator {
public:
	BitrateEstimator();
	virtual ~BitrateEstimator();

	virtual void Update(Timestamp at_time, DataSize amount, bool in_alr);
	virtual std::optional<RtcDataRate> bitrate() const;
	std::optional<RtcDataRate> PeekRate() const;
	virtual void ExpectFastRateChange();
private:
	float UpdateWindow(int64_t now_ms,
		int bytes,
		int rate_window_ms,
		bool* is_small_sample);

	int sum_;
	int initial_window_ms_;
	int noninitial_window_ms_;
	double uncertainty_scale_;
	double uncertainty_scale_in_alr_;
	double small_sample_uncertainty_scale_;
	DataSize small_sample_threshold_;
	RtcDataRate uncertainty_symmetry_cap_;
	RtcDataRate estimate_floor_;
	int64_t current_window_ms_;
	int64_t prev_time_ms_;
	float bitrate_estimate_kbps_;
	float bitrate_estimate_var_;

};

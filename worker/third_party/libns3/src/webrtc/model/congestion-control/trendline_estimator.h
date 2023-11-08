#ifndef TRENDLINE_ESTIMATOR_H
#define TRENDLINE_ESTIMATOR_H
#include "common.h"
#include <deque>

using namespace rtc;
struct TrendlineEstimatorSettings {
	//计算trendline的数据窗口大小，默认为20个
	static constexpr unsigned kDefaultTrendlineWindowSize = 20;

	// Sort the packets in the window. Should be redundant,
  // but then almost no cost.
	bool enable_sort = false;

	// Cap the trendline slope based on the minimum delay seen
	// in the beginning_packets and end_packets respectively.
	bool enable_cap = false;
	unsigned beginning_packets = 7;
	unsigned end_packets = 7;
	double cap_uncertainty = 0.0;

	// Size (in packets) of the window.
	unsigned window_size = kDefaultTrendlineWindowSize;
};

class TrendlineEstimator {
public:
	TrendlineEstimator();
	~TrendlineEstimator();

	// Update the estimator with a new sample. The deltas should represent deltas
	// between timestamp groups as defined by the InterArrival class.
	void Update(double recv_delta_ms,
		double send_delta_ms,
		int64_t send_time_ms,
		int64_t arrivate_time_ms,
		size_t packet_size,
		bool calculated_deltas);

	void UpdateTrendline(double recv_delta_ms,
		double send_delta_ms,
		int64_t send_time_ms,
		int64_t arrival_time_ms,
		size_t packet_size);

	BandwidthUsage State() const;

	struct PacketTiming {
		PacketTiming(double arrival_time_ms,
			double smoothed_delay_ms,
			double raw_delay_ms)
			:arrival_time_ms(arrival_time_ms),
			smoothed_delay_ms(smoothed_delay_ms),
			raw_delay_ms(raw_delay_ms) {}

		double arrival_time_ms;
		double smoothed_delay_ms;
		double raw_delay_ms;
	};

private:
	void Detect(double trend, double ts_delta, int64_t now_ms);
	void UpdateThreshold(double modified_offset, int64_t now_ms);
	int64_t time_{ 0 };
	TrendlineEstimatorSettings settings_;
	const double smoothing_coef_;
	const double threshold_gain_;
	int num_of_deltas_;
	int64_t first_arrival_time_ms_;
	
	// Exponential backoff filtering.
	double accumulated_delay_;
	double smoothed_delay_;

	std::deque<PacketTiming> delay_history_;

	const double k_up_;
	const double k_down_;
	double overusing_time_threshold_;
	double threshold_;
	double prev_modified_trend_;
	int64_t last_update_ms_;
	double prev_trend_;
	double time_over_using_;
	int overuse_counter_;
	BandwidthUsage hypothesis_;
	BandwidthUsage hypothesis_predicted_; 
};

#endif
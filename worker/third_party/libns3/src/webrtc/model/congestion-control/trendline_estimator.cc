#include "trendline_estimator.h"
#include <assert.h>
// Parameters for linear least squares fit of regression line to noisy data.
constexpr double kDefaultTrendlineSmoothingCoeff = 0.9;
constexpr double kDefaultTrendlineThresholdGain = 4.0;

constexpr double kMaxAdaptOffsetMs = 15.0;
constexpr double kOverUsingTimeThreshold = 10;
constexpr int kMinNumDeltas = 60;
constexpr int kDeltaCounterMax = 1000;

std::optional<double> LinearFitSlope(const std::deque<TrendlineEstimator::PacketTiming>& packets) {
	assert(packets.size() >= 2);
	// Compute the "center of mass".
	double sum_x = 0;
	double sum_y = 0;
	for (const auto& packet : packets) {
		sum_x += packet.arrival_time_ms;
		sum_y += packet.smoothed_delay_ms;
	}
	double x_avg = sum_x / packets.size();
	double y_avg = sum_y / packets.size();
	// Compute the slope k = \sum (x_i-x_avg)(y_i-y_avg) / \sum (x_i-x_avg)^2
	double numerator = 0;
	double denominator = 0;
	for (const auto& packet : packets) {
		double x = packet.arrival_time_ms;
		double y = packet.smoothed_delay_ms;
		numerator += (x - x_avg) * (y - y_avg);
		denominator += (x - x_avg) * (x - x_avg);
	}
	if (denominator == 0) {
		return std::nullopt;
	}
	return numerator / denominator;
}

std::optional<double> ComputeSlopeCap(const std::deque<TrendlineEstimator::PacketTiming>& packets,
	const TrendlineEstimatorSettings& settings) {
	assert(1 <= settings.beginning_packets &&
		settings.beginning_packets < packets.size());
	assert(1 <= settings.end_packets &&
		settings.end_packets < packets.size());
	assert(settings.beginning_packets + settings.end_packets <=
		packets.size());
	TrendlineEstimator::PacketTiming early = packets[0];
	for (size_t i = 1; i < settings.beginning_packets; ++i) {
		if (packets[i].raw_delay_ms < early.raw_delay_ms)
			early = packets[i];
	}

	size_t late_start = packets.size() - settings.end_packets;
	TrendlineEstimator::PacketTiming late = packets[late_start];
	for (size_t i = late_start + 1; i < packets.size(); ++i) {
		if (packets[i].raw_delay_ms < late.raw_delay_ms)
			late = packets[i];
	}
	if (late.arrival_time_ms - early.arrival_time_ms < 1) {
		return std::nullopt;
	}
	return (late.raw_delay_ms - early.raw_delay_ms) /
		(late.arrival_time_ms - early.arrival_time_ms) +
		settings.cap_uncertainty;
}

#define TRENDLINE_LOG
#ifdef TRENDLINE_LOG
static FILE* trendline_delay_log = nullptr;
#endif
TrendlineEstimator::TrendlineEstimator()
:smoothing_coef_(kDefaultTrendlineSmoothingCoeff),
threshold_gain_(kDefaultTrendlineThresholdGain),
num_of_deltas_(0),
first_arrival_time_ms_(-1),
accumulated_delay_(0),
smoothed_delay_(0),
delay_history_(),
k_up_(0.0087),
k_down_(0.039),
overusing_time_threshold_(kOverUsingTimeThreshold),
threshold_(12.5),
prev_modified_trend_(NAN),
last_update_ms_(-1),
prev_trend_(0.0),
time_over_using_(-1),
overuse_counter_(0),
hypothesis_(BandwidthUsage::kBwNormal),
hypothesis_predicted_(BandwidthUsage::kBwNormal)
{
#ifdef TRENDLINE_LOG
	trendline_delay_log = nullptr;
	if (trendline_delay_log == nullptr) {
		trendline_delay_log = fopen("D:\\webrtc-logs\\trendline\\delay_log.txt", "wt+");
		fprintf(trendline_delay_log, "time,accu_delay,smooth_delay,trend,modify_trend,threshold,bandwidth_use\n");
	}
#endif
}

TrendlineEstimator::~TrendlineEstimator() {
#ifdef TRENDLINE_LOG    
	if (trendline_delay_log != nullptr) {
		fclose(trendline_delay_log);
		trendline_delay_log = nullptr;
	}
#endif
}

//send time, arrival time, size都是最后这一个packet的信息，前面的delta是两个group之间的delta
void TrendlineEstimator::Update(double recv_delta_ms, double send_delta_ms,
	int64_t send_time_ms, int64_t arrival_time_ms, size_t packet_size, bool calculated_deltas) {
	if (calculated_deltas) {
		UpdateTrendline(recv_delta_ms, send_delta_ms, send_time_ms, arrival_time_ms, packet_size);
	}
}

void TrendlineEstimator::UpdateTrendline(
	double recv_delta_ms, //本group和上一个group的到达时间差
	double send_delta_ms, //发送时间差
	int64_t send_time_ms, //本group的发送时间
	int64_t arrival_time_ms, //本group的接收时间
	size_t packet_size) {
	const double delta_ms = recv_delta_ms - send_delta_ms;
	std::cout << delta_ms  << "ms" << std::endl;
	++num_of_deltas_;
	num_of_deltas_ = std::min(num_of_deltas_, kDeltaCounterMax);
	//记录第一个packet的接收时间
	if (first_arrival_time_ms_ == -1) {
		first_arrival_time_ms_ = arrival_time_ms;
	}
	// Exponential backoff filter.
	//累积延迟
	accumulated_delay_ += delta_ms;
	smoothed_delay_ = smoothing_coef_ * smoothed_delay_ +
		(1 - smoothing_coef_) * accumulated_delay_;
	//Maintain packet window
	std::cout << "累积延迟 = " << accumulated_delay_ << " 累积平滑延迟 = " << smoothed_delay_ << std::endl;

#ifdef TRENDLINE_LOG
	time_ = arrival_time_ms - first_arrival_time_ms_;
#endif
	delay_history_.emplace_back(
		static_cast<double>(arrival_time_ms - first_arrival_time_ms_),
		smoothed_delay_,
		accumulated_delay_);
	if (settings_.enable_sort) {
		for (size_t i = delay_history_.size() - 1;
			i > 0 &&
			delay_history_[i].arrival_time_ms < delay_history_[i - 1].arrival_time_ms; --i) {
			std::swap(delay_history_[i], delay_history_[i - 1]);
		}
	}
	if (delay_history_.size() > settings_.window_size) {
		delay_history_.pop_front();
	}
	//simple linear regression
	double trend = prev_trend_;
	if (delay_history_.size() == settings_.window_size) {
		// Update trend_ if it is possible to fit a line to the data. The delay
		// trend can be seen as an estimate of (send_rate - capacity)/capacity.
		// 0 < trend < 1   ->  the delay increases, queues are filling up
		//   trend == 0    ->  the delay does not change
		//   trend < 0     ->  the delay decreases, queues are being emptied
		trend = LinearFitSlope(delay_history_).value_or(trend);
		if (settings_.enable_cap) {
			std::optional<double> cap = ComputeSlopeCap(delay_history_, settings_);
			// We only use the cap to filter out overuse detections, not
			// to detect additional underuses.
			if (trend >= 0 && cap.has_value() && trend > cap.value()) {
				trend = cap.value();
			}
		}
	}
	//计算出了trend，根据这个trend来判断带宽使用状态
	Detect(trend, send_delta_ms, arrival_time_ms);
}

BandwidthUsage TrendlineEstimator::State() const {
	return hypothesis_;
}

void TrendlineEstimator::Detect(double trend, double ts_delta, int64_t now_ms) {
	if (num_of_deltas_ < 2) {
		hypothesis_ = BandwidthUsage::kBwNormal;
		return;
	}
	//放大的方法为60 * trend * 4(也就是放大了240倍）
	const double modified_trend =
		std::min(num_of_deltas_, kMinNumDeltas) * trend * threshold_gain_;
	prev_modified_trend_ = modified_trend;
	std::cout << "modified trend = " << modified_trend << "; threshold = " << threshold_ << std::endl;
	if (modified_trend > threshold_) {
		if (time_over_using_ == -1) {
			// Initialize the timer. Assume that we've been
			// over-using half of the time since the previous
			 // sample.
			time_over_using_ = ts_delta / 2;
		}
		else {
			// Increment timer 这个ts_delta是两个group的send delta
			time_over_using_ += ts_delta;
		}
		overuse_counter_++;
		//如果trend的这个趋势持续了10ms以上，则认为带宽使用处于overusing状态
		if (time_over_using_ > overusing_time_threshold_ && overuse_counter_ > 1) {
			if (trend >= prev_trend_) {
				time_over_using_ = 0;
				overuse_counter_ = 0;
				hypothesis_ = BandwidthUsage::kBwOverusing;
			}
		}
	}
	else if (modified_trend < -threshold_) {
		time_over_using_ = -1;
		overuse_counter_ = 0;
		hypothesis_ = BandwidthUsage::kBwUnderusing;
	}
	else {
		time_over_using_ = -1;
		overuse_counter_ = 0;
		hypothesis_ = BandwidthUsage::kBwNormal;
	}
	prev_trend_ = trend;

#ifdef TRENDLINE_LOG
	if (trendline_delay_log != nullptr) {
		fprintf(trendline_delay_log, "%ld,%.2f,%.2f,%.5f,%.5f,%.2f,%d\n", time_,
			accumulated_delay_, smoothed_delay_, trend, modified_trend, threshold_, (int)hypothesis_);
	}
#endif
	UpdateThreshold(modified_trend, now_ms);
	//std::cout << "trendline threshold = " << threshold_ << " modified trend = " << modified_trend << std::endl;
	if (hypothesis_ == BandwidthUsage::kBwOverusing) {
		std::cout << "带宽使用状态：Overuse！" << std::endl;
	}
	if (hypothesis_ == BandwidthUsage::kBwNormal) {
		std::cout << "带宽使用状态：Normal！" << std::endl;
	}
	if (hypothesis_ == BandwidthUsage::kBwUnderusing) {
		std::cout << "带宽使用状态：Underuse！" << std::endl;
	}
}

void TrendlineEstimator::UpdateThreshold(double modified_trend, int64_t now_ms) {
	if (last_update_ms_ == -1) {
		last_update_ms_ = now_ms;
	}
	if (fabs(modified_trend) > threshold_ + kMaxAdaptOffsetMs) {
		// Avoid adapting the threshold to big latency spikes, caused e.g.,
		// by a sudden capacity drop.
		last_update_ms_ = now_ms;
		return;
	}
	const double k = fabs(modified_trend) < threshold_ ? k_down_ : k_up_;
	const int64_t kMaxTimeDeltaMs = 100;
	int64_t time_delta_ms = std::min(now_ms - last_update_ms_, kMaxTimeDeltaMs);
	threshold_ += k * (fabs(modified_trend) - threshold_) * time_delta_ms;
	if (threshold_ > 600.f) {
		threshold_ = 600.f;
	}
	if (threshold_ < 6.f) {
		threshold_ = 6.f;
	}	
	std::cout << "更新之后的threshold = " << threshold_ << std::endl;
	last_update_ms_ = now_ms;
}
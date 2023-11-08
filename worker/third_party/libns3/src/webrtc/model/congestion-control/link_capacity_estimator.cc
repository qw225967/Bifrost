#include "link_capacity_estimator.h"
#include <algorithm>

LinkCapacityEstimator::LinkCapacityEstimator() {}
LinkCapacityEstimator::~LinkCapacityEstimator() {}

//是按照正态分布来估算上界和下界，均值+-3倍方差
RtcDataRate LinkCapacityEstimator::UpperBound() const {
	if (estimate_kbps_.has_value()) {
		return RtcDataRate::KilobitsPerSec(estimate_kbps_.value() +
			3 * deviation_estimate_kbps());
	}
	return RtcDataRate::Infinity();
}

RtcDataRate LinkCapacityEstimator::LowerBound() const {
	if (estimate_kbps_.has_value())
		return RtcDataRate::KilobitsPerSec(
			std::max(0.0, estimate_kbps_.value() - 3 * deviation_estimate_kbps()));
	return RtcDataRate::Zero();
}

void LinkCapacityEstimator::Reset() {
	estimate_kbps_.reset();
}

void LinkCapacityEstimator::OnOveruseDetected(RtcDataRate acknowledged_rate) {
	Update(acknowledged_rate, 0.5);
}

void LinkCapacityEstimator::OnProbeRate(RtcDataRate probe_rate) {
	Update(probe_rate, 0.5);
}

void LinkCapacityEstimator::Update(RtcDataRate capacity_sample, double alpha) {
	double sample_kbps = capacity_sample.kbps();
	if (!estimate_kbps_.has_value()) {
		estimate_kbps_ = sample_kbps;
	}
	else {
		estimate_kbps_ = (1 - alpha) * estimate_kbps_.value() + alpha * sample_kbps;
	}
	// Estimate the variance of the link capacity estimate and normalize the
  // variance with the link capacity estimate.
	const double norm = std::max(estimate_kbps_.value(), 1.0);
	double error_kbps = estimate_kbps_.value() - sample_kbps;
	deviation_kbps_ =
		(1 - alpha) * deviation_kbps_ + alpha * error_kbps * error_kbps / norm;
	// 0.4 ~= 14 kbit/s at 500 kbit/s
	// 2.5f ~= 35 kbit/s at 500 kbit/s
	//这个地方的更新计算的意义和价值在哪里呢？否则增长码率的限制就可能不合理
	if (deviation_kbps_ > 2.5f) {
		deviation_kbps_ = 2.5f;
	}
	if (deviation_kbps_ < 0.4f) {
		deviation_kbps_ = 0.4f;
	}
}

bool LinkCapacityEstimator::has_estimate() const {
	return estimate_kbps_.has_value();
}

RtcDataRate LinkCapacityEstimator::estimate() const {
	return RtcDataRate::KilobitsPerSec(*estimate_kbps_);
}

double LinkCapacityEstimator::deviation_estimate_kbps() const {
	// Calculate the max bit rate std dev given the normalized
	// variance and the current throughput bitrate. The standard deviation will
	// only be used if estimate_kbps_ has a value.
	return sqrt(deviation_kbps_ * estimate_kbps_.value());
}
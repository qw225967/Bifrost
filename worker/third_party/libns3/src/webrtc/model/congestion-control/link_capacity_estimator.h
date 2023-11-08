#ifndef LINK_CAPACITY_ESTIMATOR_H
#define LINK_CAPACITY_ESTIMATOR_H
#include <optional>
#include "ns3/data_rate.h"

using namespace rtc;
class LinkCapacityEstimator {
public:
	LinkCapacityEstimator();
	~LinkCapacityEstimator();

	RtcDataRate UpperBound() const;
	RtcDataRate LowerBound() const;

	void Reset();
	void OnOveruseDetected(RtcDataRate acknowledged_rate);
	void OnProbeRate(RtcDataRate probe_rate);
	bool has_estimate() const;
	RtcDataRate estimate() const;

private:
	void Update(RtcDataRate capacity_sample, double alpha);
	double deviation_estimate_kbps() const;
	std::optional<double> estimate_kbps_;
	double deviation_kbps_ = 0.4;
};

#endif
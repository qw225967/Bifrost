#include "delay_based_bwe.h"

constexpr TimeDelta kStreamTimeOut = TimeDelta::Seconds(2);
constexpr TimeDelta kSendTimeGroupLength = TimeDelta::Millis(5);

/* std::unique_ptr<InterArrivalDelta> inter_arrival_delta_;
	std::unique_ptr<TrendlineEstimator> trendline_estimator_;
	AimdRateController rate_controller_;

	Timestamp last_seen_packet_;
	RtcDataRate prev_bitrate_;
	bool has_once_detected_overuse_;
	BandwidthUsage prev_state_;
	bool alr_limited_backoff_enabled_;*/

DelayBasedBwe::Result::Result()
	:updated(false),
	probe(false),
	target_bitrate(RtcDataRate::Zero()),
	recovered_from_overuse(false),
	backoff_in_alr(false) {}

DelayBasedBwe::DelayBasedBwe()
:prev_bitrate_(RtcDataRate::Zero()),
last_seen_packet_(Timestamp::PlusInfinity()),
prev_state_(BandwidthUsage::kBwNormal),
has_once_detected_overuse_(false),
alr_limited_backoff_enabled_(true),
trendline_estimator_(new TrendlineEstimator())
{}
DelayBasedBwe::~DelayBasedBwe() {}

DelayBasedBwe::Result DelayBasedBwe::IncomingPacketFeedbackVector(const TransportPacketsFeedback& msg,
	std::optional<RtcDataRate> acked_bitrate,
	std::optional<RtcDataRate> probe_bitrate,
	bool in_alr){
	auto packet_feedback_vector = msg.SortedByReceiveTime();
	if (packet_feedback_vector.empty()) {
		return DelayBasedBwe::Result();
	}	
	
	bool recovered_from_overuse = false;
	BandwidthUsage prev_detector_state = trendline_estimator_->State(); //上次检测的状态
	for (const auto& packet_feedback : packet_feedback_vector) {		
		IncomingPacketFeedback(packet_feedback, msg.feedback_time);
		//针对一个反馈的packet利用trendline进行估计，如果估计的状态从underusing变为normal，则设定recovered_from_overuse为true
		if (prev_detector_state == BandwidthUsage::kBwUnderusing &&
			trendline_estimator_->State() == BandwidthUsage::kBwNormal) {
			recovered_from_overuse = true;
		}
		//状态变换了
		prev_detector_state = trendline_estimator_->State();
	}
	rate_controller_.SetInApplicationLimitedRegion(in_alr);
	//根据检测状态确定速率的估计（该增加还是减少...)
	return MaybeUpdateEstimate(acked_bitrate, probe_bitrate, recovered_from_overuse, in_alr, msg.feedback_time);
}

void DelayBasedBwe::IncomingPacketFeedback(const PacketResult& packet_feedback, Timestamp at_time) {
	// Reset if the stream has timed out.
	if (last_seen_packet_.IsInfinite() || at_time - last_seen_packet_ > kStreamTimeOut) {
		inter_arrival_delta_ = std::make_unique<InterArrivalDelta>(kSendTimeGroupLength);
		//trendline_estimator_.reset(new TrendlineEstimator());
	}
	last_seen_packet_ = at_time;

	DataSize packet_size = packet_feedback.sent_packet.size;
	TimeDelta send_delta = TimeDelta::Zero();
	TimeDelta recv_delta = TimeDelta::Zero();
	int size_delta = 0;
	bool calculated_deltas = inter_arrival_delta_->ComputeDeltas(
		packet_feedback.sent_packet.send_time, packet_feedback.receive_time,
		at_time, packet_size.bytes(), &send_delta, &recv_delta, &size_delta);

	trendline_estimator_->Update((int64_t)recv_delta.ms(), (int64_t)send_delta.ms(),
		(int64_t)packet_feedback.sent_packet.send_time.ms(), (int64_t)packet_feedback.receive_time.ms(),
		(int64_t)packet_size.bytes(), calculated_deltas);
}

DelayBasedBwe::Result DelayBasedBwe::MaybeUpdateEstimate(
	std::optional<RtcDataRate> acked_bitrate,
	std::optional<RtcDataRate> probe_bitrate,
	bool recovered_from_overuse,
	bool in_alr,
	Timestamp at_time) {
	Result result;
	// Currently overusing the bandwidth.
	if (trendline_estimator_->State() == BandwidthUsage::kBwOverusing) {
		if (has_once_detected_overuse_ && in_alr && alr_limited_backoff_enabled_) {
			result.updated =
				UpdateEstimate(at_time, prev_bitrate_, &result.target_bitrate);
			result.backoff_in_alr = true;
		}
		else if (acked_bitrate && rate_controller_.TimeToReduceFurther(at_time, *acked_bitrate)) {
			result.updated = UpdateEstimate(at_time, acked_bitrate, &result.target_bitrate);
		}
		else if (!acked_bitrate && rate_controller_.ValidEstimate() &&
			rate_controller_.InitialTimeToReduceFurther(at_time)) {
			// Overusing before we have a measured acknowledged bitrate. Reduce send
			// rate by 50% every 200 ms.
			// TODO(tschumim): Improve this and/or the acknowledged bitrate estimator
			// so that we (almost) always have a bitrate estimate.
			rate_controller_.SetEstimate(rate_controller_.LatestEstimate() / 2, at_time);
			result.updated = true;
			result.probe = false;
			result.target_bitrate = rate_controller_.LatestEstimate();
		}
		has_once_detected_overuse_ = true;
	}
	else {
		if (probe_bitrate) {
			result.probe = true;
			result.updated = true;
			result.target_bitrate = *probe_bitrate;
			rate_controller_.SetEstimate(*probe_bitrate, at_time);
		}
		else {
			result.updated = UpdateEstimate(at_time, acked_bitrate, &result.target_bitrate);
			result.recovered_from_overuse = recovered_from_overuse;
		}
	}
	BandwidthUsage detector_state = trendline_estimator_->State();
	if ((result.updated && prev_bitrate_ != result.target_bitrate) ||
		detector_state != prev_state_) {
		RtcDataRate bitrate = result.updated ? result.target_bitrate : prev_bitrate_;
		prev_bitrate_ = bitrate;
		prev_state_ = detector_state;
	}
	//std::cout << "delay based bwe bitrate = " << result.target_bitrate.bps() << std::endl;
	return result;
}

bool DelayBasedBwe::UpdateEstimate(Timestamp at_time,
	std::optional<RtcDataRate> acked_bitrate,
	RtcDataRate* target_rate) {
	const RateControlInput input(trendline_estimator_->State(), acked_bitrate);
	*target_rate = rate_controller_.Update(&input, at_time);
	return rate_controller_.ValidEstimate();
}

void DelayBasedBwe::OnRttUpdate(TimeDelta avg_rtt) {
	rate_controller_.SetRtt(avg_rtt);
}

bool DelayBasedBwe::LatestEstimate(RtcDataRate* bitrate) const {
	// Currently accessed from both the process thread (see
	// ModuleRtpRtcpImpl::Process()) and the configuration thread (see
	// Call::GetStats()). Should in the future only be accessed from a single
	// thread.
	
	if (!rate_controller_.ValidEstimate())
		return false;
	
	*bitrate = rate_controller_.LatestEstimate();
	return true;
}

void DelayBasedBwe::SetStartBitrate(RtcDataRate start_bitrate) {	
	rate_controller_.SetStartBitrate(start_bitrate);
}

void DelayBasedBwe::SetMinBitrate(RtcDataRate min_bitrate) {
	// Called from both the configuration thread and the network thread. Shouldn't
	// be called from the network thread in the future.
	rate_controller_.SetMinBitrate(min_bitrate);
}

TimeDelta DelayBasedBwe::GetExpectedBwePeriod() const {
	return rate_controller_.GetExpectedBandwidthPeriod();
}

void DelayBasedBwe::SetAlrLimitedBackoffExperiment(bool enabled) {
	alr_limited_backoff_enabled_ = enabled;
}



#include "aimd_rate_controller.h"
#include <assert.h>
constexpr TimeDelta kDefaultRtt = TimeDelta::Millis(200);
constexpr double kDefaultBackoffFactor = 0.85;

AimdRateController::AimdRateController()
	:min_configured_bitrate_(RtcDataRate::KilobitsPerSec(300)),
	max_configured_bitrate_(RtcDataRate::KilobitsPerSec(30000)),
	current_bitrate_(max_configured_bitrate_),
	latest_estimated_throughput_(current_bitrate_),
	link_capacity_(),
	rate_control_state_(RateControlState::kRcHold),
	time_last_bitrate_change_(Timestamp::MinusInfinity()),
	time_last_bitrate_decrease_(Timestamp::MinusInfinity()),
	time_first_throughput_estimate_(Timestamp::MinusInfinity()),
	bitrate_is_initialized_(false),
	beta_(kDefaultBackoffFactor),
	in_alr_(false),
	rtt_(kDefaultRtt),
	no_bitrate_increase_in_alr_(false),
	estimate_bounded_backoff_(false),
	estimate_bounded_increase_(false)
{}

AimdRateController::~AimdRateController() {}

void AimdRateController::SetStartBitrate(RtcDataRate start_bitrate) {
	current_bitrate_ = start_bitrate;
	latest_estimated_throughput_ = current_bitrate_;
	bitrate_is_initialized_ = true;
}

void AimdRateController::SetMinBitrate(RtcDataRate min_bitrate) {
	min_configured_bitrate_ = min_bitrate;
	current_bitrate_ = std::max(min_bitrate, current_bitrate_);
}

bool AimdRateController::ValidEstimate() const {
	return bitrate_is_initialized_;
}

//bitrate降低间隔在（10,100）ms之间，一般取一个rtt
//acked bitrate < threshold, 也就是说acked bitrate降低了一半
bool AimdRateController::TimeToReduceFurther(Timestamp at_time,
	RtcDataRate estimated_throughput) const {
	const TimeDelta bitrate_reduction_interval =
		rtt_.Clamped(TimeDelta::Millis(10), TimeDelta::Millis(200));
	if (at_time - time_last_bitrate_change_ >= bitrate_reduction_interval) {
		return true;
	}
	//如果估计的带宽下降太厉害，还是要降低码率的（就先不管时间是否满足了）
	if (ValidEstimate()) {
		const RtcDataRate threshold = 0.5 * LatestEstimate();
		return estimated_throughput < threshold;
	}
	return false;
}
//如果在初始化阶段，还没有ACK码率，如果遇到了overuse，我们应该降低码率。需不需要等一段时间？
//AIMD中通过initial_backoff_interval_这个配置项来控制。
//InitialTimeToReduceFurther这个函数就是用来判断，初始化阶段是否立即降低码率。
bool AimdRateController::InitialTimeToReduceFurther(Timestamp at_time) const {
	// 没有设置interval，那么在初始化阶段一定会退避，遇到overuse就降低码率
	//不太理解后面这个地方，为啥LatestEstimate()/2 - 1 ？
	//estimated_throughput是ack bitrate
	//说明：LatestEstimate()/2-1一定小于了LatestEstimate()/2，所以会返回true，也就是一定会reduce
	return ValidEstimate() &&
		TimeToReduceFurther(at_time, LatestEstimate() / 2 - RtcDataRate::BitsPerSec(1));
}

RtcDataRate AimdRateController::LatestEstimate() const {
	return current_bitrate_;
}

void AimdRateController::SetRtt(TimeDelta rtt) {
	rtt_ = rtt;
}

RtcDataRate AimdRateController::Update(const RateControlInput* input, Timestamp at_time) {
	assert(input != nullptr);
	//什么情况下会进入到这里？
	if (!bitrate_is_initialized_) {
		const TimeDelta kInitializationTime = TimeDelta::Seconds(5);
		assert(kBitrateWindowMs <= kInitializationTime.ms());
		//以前没有acked bitrate
		if (time_first_throughput_estimate_.IsInfinite()) {
			//这次有acked bitrate了
			if (input->estimated_throughput) {
				time_first_throughput_estimate_ = at_time;
			}
		}
		else if (at_time - time_first_throughput_estimate_ > kInitializationTime &&
			input->estimated_throughput) {
			current_bitrate_ = *input->estimated_throughput;
			bitrate_is_initialized_ = true;
		}
	}
	ChangeBitrate(*input, at_time);
	return current_bitrate_;
}

void AimdRateController::SetInApplicationLimitedRegion(bool in_alr) {
	in_alr_ = in_alr;
}

//设置bitrate，记录bitrate更新时间，并记录bitrate降低时间
void AimdRateController::SetEstimate(RtcDataRate bitrate, Timestamp at_time) {
	bitrate_is_initialized_ = true;
	RtcDataRate prev_bitrate = current_bitrate_;
	current_bitrate_ = ClampBitrate(bitrate);
	time_last_bitrate_change_ = at_time;
	if (current_bitrate_ < prev_bitrate) {
		time_last_bitrate_decrease_ = at_time;
	}
}

//4k一个（rtt_ + 100ms），这个数值值得再推敲
double AimdRateController::GetNearMaxIncreaseRateBpsPerSecond() const {
	assert(!current_bitrate_.IsZero());
	const TimeDelta kFrameInterval = TimeDelta::Seconds(1) / 30;
	DataSize frame_size = current_bitrate_ * kFrameInterval;
	const DataSize kPacketSize = DataSize::Bytes(1200);
	double packets_per_frame = std::ceil(frame_size / kPacketSize);
	DataSize avg_packet_size = frame_size / packets_per_frame;

	// Approximate the over-use estimator delay to 100 ms.
	//1个response time增加一个packet，这个response time是不是太长了？
	TimeDelta response_time = rtt_ + TimeDelta::Millis(100);
	double increase_rate_bps_per_second =
		(avg_packet_size / response_time).bps<double>();

	double kMinIncreaseRateBpsPerSecond = 4000;
	return std::max(kMinIncreaseRateBpsPerSecond, increase_rate_bps_per_second);
}

//返回两次overuse信号之间的expected time（假设稳态）
TimeDelta AimdRateController::GetExpectedBandwidthPeriod() const {
	const TimeDelta kMinPeriod = TimeDelta::Seconds(2);
	const TimeDelta kDefaultPeriod = TimeDelta::Seconds(3);
	const TimeDelta kMaxPeriod = TimeDelta::Seconds(50);

	double increase_rate_bps_per_second = GetNearMaxIncreaseRateBpsPerSecond();
	if (!last_decrease_) {
		return kDefaultPeriod;
	}

	double time_to_recover_decrease_seconds =
		last_decrease_->bps() / increase_rate_bps_per_second;

	TimeDelta period = TimeDelta::Seconds(time_to_recover_decrease_seconds);
	return period.Clamped(kMinPeriod, kMaxPeriod);
}

void AimdRateController::ChangeState(const RateControlInput& input, Timestamp at_time) {
	switch (input.bw_state) {
	//带宽状态kBwNormal（如果前面是kRcHold，则可以增加码率，探测一下）
	case BandwidthUsage::kBwNormal: 
		if (rate_control_state_ == RateControlState::kRcHold) {
			time_last_bitrate_change_ = at_time;
			rate_control_state_ = RateControlState::kRcIncrease;
		}
		break;
	//decrease一次后会进入hold状态
	case BandwidthUsage::kBwOverusing:
		if (rate_control_state_ != RateControlState::kRcDecrease) {
			rate_control_state_ = RateControlState::kRcDecrease;
		}
		break;
	//利用率低说明码率不足，带宽利用率的问题
	case BandwidthUsage::kBwUnderusing:
		rate_control_state_ = RateControlState::kRcHold;
		break;
	default:
		break;
	}
}

RtcDataRate AimdRateController::ClampBitrate(RtcDataRate new_bitrate) const {
	new_bitrate = std::max(new_bitrate, min_configured_bitrate_);
	return new_bitrate;
}

RtcDataRate AimdRateController::MultiplicativeRateIncrease(
	Timestamp at_time,
	Timestamp last_time,
	RtcDataRate current_bitrate) const {
	double alpha = 1.08;
	if (last_time.IsFinite()) {
		auto time_since_last_update = at_time - last_time;
		alpha = pow(alpha, std::min(time_since_last_update.seconds<double>(), 1.0));
	}
	RtcDataRate multiplicative_increase =
		std::max(current_bitrate * (alpha - 1.0), RtcDataRate::BitsPerSec(1000));
	return multiplicative_increase;
}

RtcDataRate AimdRateController::AdditiveRateIncrease(Timestamp at_time, Timestamp last_time) const {
	double time_period_seconds = (at_time - last_time).seconds<double>();
	double data_rate_increase_bps =
		GetNearMaxIncreaseRateBpsPerSecond() * time_period_seconds;
	return RtcDataRate::BitsPerSec(data_rate_increase_bps);
}

void AimdRateController::ChangeBitrate(const RateControlInput& input, Timestamp at_time) {
	std::optional<RtcDataRate> new_bitrate;
	RtcDataRate estimated_throughput =
		input.estimated_throughput.value_or(latest_estimated_throughput_);
	if (input.estimated_throughput)
		latest_estimated_throughput_ = *input.estimated_throughput;

	// An over-use should always trigger us to reduce the bitrate, even though
	// we have not yet established our first estimate. By acting on the over-use,
	 // we will end up with a valid estimate.
	if (!bitrate_is_initialized_ &&
		input.bw_state != BandwidthUsage::kBwOverusing) {
		return;
	}
	ChangeState(input, at_time);

	// We limit the new bitrate based on the troughput to avoid unlimited bitrate
	// increases. We allow a bit more lag at very low rates to not too easily get
	// stuck if the encoder produces uneven outputs.
	const RtcDataRate throughput_based_limit =
		1.5 * estimated_throughput + RtcDataRate::KilobitsPerSec(10);

	switch (rate_control_state_) {
	case RateControlState::kRcHold:
		break;
	case RateControlState::kRcIncrease:
		if (estimated_throughput > link_capacity_.UpperBound())
			link_capacity_.Reset();

		// Do not increase the delay based estimate in alr since the estimator
		// will not be able to get transport feedback necessary to detect if
		// the new estimate is correct.
		// If we have previously increased above the limit (for instance due to
		// probing), we don't allow further changes.
		if (current_bitrate_ < throughput_based_limit && !(in_alr_ && no_bitrate_increase_in_alr_)) {
			RtcDataRate increased_bitrate = RtcDataRate::MinusInfinity();
			if (link_capacity_.has_estimate()) { //说明没有reset
				// The link_capacity estimate is reset if the measured throughput
				// is too far from the estimate. We can therefore assume that our
				// target rate is reasonably close to link capacity and use additive
				// increase.
				RtcDataRate additive_increase =
					AdditiveRateIncrease(at_time, time_last_bitrate_change_);
				increased_bitrate = current_bitrate_ + additive_increase;
			}
			else {
				// If we don't have an estimate of the link capacity, use faster ramp
				// up to discover the capacity.
				RtcDataRate multiplicative_increase = MultiplicativeRateIncrease(at_time, time_last_bitrate_change_, current_bitrate_);
				increased_bitrate = current_bitrate_ + multiplicative_increase;
			}
			new_bitrate = std::min(increased_bitrate, throughput_based_limit);
		}
		time_last_bitrate_change_ = at_time;
		break;
	case RateControlState::kRcDecrease: {
		RtcDataRate decreased_bitrate = RtcDataRate::PlusInfinity();
		// Set bit rate to something slightly lower than the measured throughput
		// to get rid of any self-induced delay.
		decreased_bitrate = estimated_throughput * beta_; //基于吞吐量的乘性下降
		//如果下降之后仍然大于current bitrate, 则使用Link capacity的估计值来乘性下降
		if (decreased_bitrate > current_bitrate_) {
			if (link_capacity_.has_estimate()) {
				decreased_bitrate = beta_ * link_capacity_.estimate();
			}
		}
		// Avoid increasing the rate when over-using.
		if (decreased_bitrate < current_bitrate_) {
			new_bitrate = decreased_bitrate;
		}
		//上次下降了多少
		if (bitrate_is_initialized_ && estimated_throughput < current_bitrate_) {
			if (!new_bitrate.has_value()) {
				last_decrease_ = RtcDataRate::Zero();
			}
			else {
				last_decrease_ = current_bitrate_ - *new_bitrate;
			}
		}
		//如果估计的吞吐量低于链路容量下界，则需要重新估计链路容量
		if (estimated_throughput < link_capacity_.LowerBound()) {
			// The current throughput is far from the estimated link capacity. Clear
			// the estimate to allow an immediate update in OnOveruseDetected.
			link_capacity_.Reset();
		}

		bitrate_is_initialized_ = true;
		//下降说明over used了
		link_capacity_.OnOveruseDetected(estimated_throughput);
		// Stay on hold until the pipes are cleared.
		//速率控制状态为hold，就是等待排干
		rate_control_state_ = RateControlState::kRcHold;
		time_last_bitrate_change_ = at_time;
		time_last_bitrate_decrease_ = at_time;
		break;
	}
	default:
		break;
		
	}
	current_bitrate_ = ClampBitrate(new_bitrate.value_or(current_bitrate_));
}





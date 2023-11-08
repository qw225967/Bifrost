#include "inter_arrival_delta.h"
#include <iostream>
using namespace rtc;
static constexpr TimeDelta kBurstDeltaThreshold = TimeDelta::Millis(5);
static constexpr TimeDelta kMaxBurstDuration = TimeDelta::Millis(100);
constexpr TimeDelta InterArrivalDelta::kArrivalTimeOffsetThreshold;

InterArrivalDelta::InterArrivalDelta(TimeDelta send_time_group_length)
	:send_time_group_length_(send_time_group_length),
	current_timestamp_group_(),
	prev_timestamp_group_(),
	num_consecutive_reordered_packets_(0){}

//一定要考虑burst的判断对于整个计算的影响
bool InterArrivalDelta::BelongsToBurst(Timestamp arrival_time, Timestamp send_time) const {
    assert(current_timestamp_group_.complete_time.IsFinite());
    //和上一个packet的到达时间间隔
    TimeDelta arrival_time_delta = arrival_time - current_timestamp_group_.complete_time;
    TimeDelta send_time_delta = send_time - current_timestamp_group_.send_time;
    //突发发送
    if (send_time_delta.IsZero()) {
        return true;
    }
    TimeDelta propagation_delta = arrival_time_delta - send_time_delta;
    //在中间队列上的突发（导致到达时间的delta小于send time的delta),因为按照一般的来说应该arrival delta>= send delta
    if (propagation_delta < TimeDelta::Zero() &&
        arrival_time_delta <= kBurstDeltaThreshold && //两个packet的到达时间差小于5ms
        arrival_time - current_timestamp_group_.first_arrival_time < kMaxBurstDuration) { //和当前group第一个packet的到达时间之差小于100ms
        return true;
    }
    return false;
}

// Assumes that `timestamp` is not reordered compared to
// `current_timestamp_group_`.
bool InterArrivalDelta::NewTimestampGroup(Timestamp arrival_time, Timestamp send_time) const {
    if (current_timestamp_group_.IsFirstPacket()) {
        return false;
    }
    else if (BelongsToBurst(arrival_time, send_time)) {
        return false;
    }
    else {
        return send_time - current_timestamp_group_.first_send_time > send_time_group_length_;
    }
}

//可以考虑在弱网模拟中构造失序传输的情形
static Timestamp last_time = Timestamp::MinusInfinity();
bool InterArrivalDelta::ComputeDeltas(
    Timestamp send_time,
    Timestamp arrival_time,
    Timestamp system_time,
    size_t packet_size,
    TimeDelta* send_time_delta,
    TimeDelta* arrival_time_delta,
    int* packet_size_delta) {
    if (last_time == Timestamp::MinusInfinity()) {
        last_time = send_time;
    }
    else {
        //std::cout << "send delta = " << (send_time - last_time).ms() << std::endl;
        last_time = send_time;
    }
    
    bool calculated_deltas = false;
    if (current_timestamp_group_.IsFirstPacket()) {
        current_timestamp_group_.send_time = send_time;
        current_timestamp_group_.first_send_time = send_time;
        current_timestamp_group_.first_arrival_time = arrival_time;
    }
    else if (current_timestamp_group_.first_send_time > send_time) {
        //失序packet
        return false;
    }
    //NewTimestampGroup表示current group要过时了
    else if (NewTimestampGroup(arrival_time, send_time)) {
        // First packet of a later send burst, the previous packets sample is ready.
        //prev group必须有效才能计算delta
        if (prev_timestamp_group_.complete_time.IsFinite()) {
            *send_time_delta = current_timestamp_group_.send_time -
                prev_timestamp_group_.send_time;
            *arrival_time_delta = current_timestamp_group_.complete_time -
                prev_timestamp_group_.complete_time;
                            
            TimeDelta system_time_delta = current_timestamp_group_.last_system_time -
                prev_timestamp_group_.last_system_time;
            if (*arrival_time_delta - system_time_delta >= kArrivalTimeOffsetThreshold) {
                std::cout << "The arrival time clock offset has changed (diff = "
                    << arrival_time_delta->ms() - system_time_delta.ms()
                    << " ms), resetting.";
                Reset();
                return false;
            }
            //两个groups到达失序了，不能用了
            if (*arrival_time_delta < TimeDelta::Zero()) {
                // The group of packets has been reordered since receiving its local
                // arrival timestamp.
                ++num_consecutive_reordered_packets_;
                if (num_consecutive_reordered_packets_ >= kReorderedResetThreshold) {
                    std::cout << "Packets between send burst arrived out of order, resetting."
                        << " arrival_time_delta" << arrival_time_delta->ms()
                        << " send time delta " << send_time_delta->ms();
                    Reset();
                }
                return false;
            }
            else {
                num_consecutive_reordered_packets_ = 0;
            }
            *packet_size_delta = static_cast<int>(current_timestamp_group_.size)
                - static_cast<int>(prev_timestamp_group_.size);
            calculated_deltas = true;
            //std::cout << "delta = " << (arrival_time_delta - send_time_delta).ms() << std::endl;
        }
        prev_timestamp_group_ = current_timestamp_group_;
        // The new timestamp is now the current frame.
        current_timestamp_group_.first_send_time = send_time;
        current_timestamp_group_.send_time = send_time;
        current_timestamp_group_.first_arrival_time = arrival_time;
        current_timestamp_group_.size = 0;
    }
    //非NewTimestampGroup
    else {
        current_timestamp_group_.send_time = std::max(
            current_timestamp_group_.send_time, send_time
        );
    }
    // Accumulate the frame size.
    current_timestamp_group_.size += packet_size;
    current_timestamp_group_.complete_time = arrival_time;
    current_timestamp_group_.last_system_time = system_time;
    
    return calculated_deltas;
}

void InterArrivalDelta::Reset() {
    num_consecutive_reordered_packets_ = 0;
    current_timestamp_group_ = SendTimeGroup();
    prev_timestamp_group_ = SendTimeGroup();
}

#include "pacing_controller.h"
#include "packet_router.h"
#include "ns3/simulator.h"
// Time limit in milliseconds between packet bursts.
constexpr TimeDelta kDefaultMinPacketLimit = TimeDelta::Millis(2);
constexpr TimeDelta kCongestedPacketInterval = TimeDelta::Millis(500);
// TODO(sprang): Consider dropping this limit.
// The maximum debt level, in terms of time, capped when sending packets.
constexpr TimeDelta kMaxDebtInTime = TimeDelta::Millis(500);
constexpr TimeDelta kMaxElapsedTime = TimeDelta::Seconds(2);

// Upper cap on process interval, in case process has not been called in a long
// time. Applies only to periodic mode.
constexpr TimeDelta kMaxProcessingInterval = TimeDelta::Millis(30);

// Allow probes to be processed slightly ahead of inteded send time. Currently
// set to 1ms as this is intended to allow times be rounded down to the nearest
// millisecond.
constexpr TimeDelta kMaxEarlyProbeProcessing = TimeDelta::Millis(1);

constexpr int kFirstPriority = 0;

int GetPriorityForType(PacketType type) {
    // Lower number takes priority over higher.
    switch (type) {    
    case PacketType::kRetransmission:
        // Send retransmissions before new media.
        return kFirstPriority + 1;
    case PacketType::kVideo:
    case PacketType::kForwardErrorCorrection:
        // Video has "normal" priority, in the old speak.
        // Send redundancy concurrently to video. If it is delayed it might have a
        // lower chance of being useful.
        return kFirstPriority + 2;
    case PacketType::kPadding:
        // Packets that are in themselves likely useless, only sent to keep the
        // BWE high.
        return kFirstPriority + 3;
    default:    
        return kFirstPriority + 10; //无奈之举
    }    
}

const TimeDelta PacingController::kMaxExpectedQueueLength =
TimeDelta::Millis(2000);
const float PacingController::kDefaultPaceMultiplier = 2.5f;
const TimeDelta PacingController::kPausedProcessInterval =
kCongestedPacketInterval;
const TimeDelta PacingController::kMinSleepTime = TimeDelta::Millis(1);

PacingController::PacingController(PacketRouter* packet_router, ProcessMode mode)
    :mode_(mode),    
    packet_sender_(packet_router),
    drain_large_queues_(true),
    send_padding_if_silent_(false),    
    ignore_transport_overhead_(true),
    padding_target_duration_(TimeDelta::Millis(5)),
    min_packet_limit_(kDefaultMinPacketLimit),
    transport_overhead_per_packet_(DataSize::Zero()),
    last_timestamp_(Timestamp::Millis(Simulator::Now().GetMilliSeconds())),
    paused_(false),
    media_budget_(0),
    padding_budget_(0),
    media_debt_(DataSize::Zero()),
    padding_debt_(DataSize::Zero()),
    media_rate_(RtcDataRate::Zero()),
    padding_rate_(RtcDataRate::Zero()),
    prober_(),
    probing_send_failure_(false),
    pacing_bitrate_(RtcDataRate::Zero()),
    last_process_time_(Timestamp::Millis(Simulator::Now().GetMilliSeconds())),
    last_send_time_(last_process_time_),
    packet_queue_(Timestamp::Zero()),
    packet_counter_(0),
    congestion_window_size_(DataSize::PlusInfinity()),
    outstanding_data_(DataSize::Zero()),
    queue_time_limit(kMaxExpectedQueueLength),
    account_for_audio_(false),
    include_overhead_(false)
{
    if (!drain_large_queues_) {
        std::cout << "Pacer queues will not be drained,"
            "pushback experiment must be enabled.";
    }
    UpdateBudgetWithElapsedTime(min_packet_limit_);
}

PacingController::~PacingController() = default;

void PacingController::CreateProbeCluster(RtcDataRate bitrate, int cluster_id) {
    prober_.CreateProbeCluster(bitrate, CurrentTime(), cluster_id);
}

void PacingController::Pause() {
    if (!paused_) {
        std::cout << "PacedSender paused.";
    }
    paused_ = true;
    packet_queue_.SetPauseState(true, CurrentTime());
}

void PacingController::Resume() {
    if (paused_) {
        std::cout << "PacedSender resumed.";
    }
    paused_ = false;
    packet_queue_.SetPauseState(false, CurrentTime());
}

bool PacingController::IsPaused() const {
    return paused_;
}

void PacingController::SetCongestionWindow(DataSize congestion_window_size) {
    const bool was_congested = Congested();
    congestion_window_size_ = congestion_window_size;
    if (was_congested && !Congested()) {
        TimeDelta elapsed_time = UpdateTimeAndGetElapsed(CurrentTime());
        UpdateBudgetWithElapsedTime(elapsed_time);
    }
}

void PacingController::UpdateOutstandingData(DataSize outstanding_data) {
    const bool was_congested = Congested();
    outstanding_data_ = outstanding_data;
    if (was_congested && !Congested()) {
        TimeDelta elapsed_time = UpdateTimeAndGetElapsed(CurrentTime());
        UpdateBudgetWithElapsedTime(elapsed_time);
    }
}

bool PacingController::Congested() const {
    if (congestion_window_size_.IsFinite()) {
        return outstanding_data_ >= congestion_window_size_;
    }
    return false;
}

bool PacingController::IsProbing() const {
    return prober_.is_probing();
}

Timestamp PacingController::CurrentTime() const {
    Timestamp time = Timestamp::Millis(Simulator::Now().GetMilliSeconds());
    if (time < last_timestamp_) {
        std::cout << "Non-monotonic clock behavior observer. Previous timestamp: " <<
            last_timestamp_.ms() << ", new timestamp: " << time.ms();
        assert(time >= last_timestamp_);
        time = last_timestamp_;
    }
    last_timestamp_ = time;
    return time;
}

void PacingController::SetProbingEnabled(bool enable) {
    assert(0 == packet_counter_);
    prober_.SetEnabled(enable);
}

void PacingController::SetPacingRates(RtcDataRate pacing_rate, RtcDataRate padding_rate) {
    assert(pacing_rate > RtcDataRate::Zero());
    media_rate_ = pacing_rate;
    padding_rate_ = padding_rate;
    pacing_bitrate_ = pacing_rate;
    padding_budget_.set_target_rate_kbps(padding_rate.kbps());
    std::cout << "bwe:pacer_updated pacing_kbps="
        << pacing_bitrate_.kbps()
        << " padding_budget_kbps="
        << padding_rate.kbps();
}

void PacingController::EnqueuePacket(std::unique_ptr<PacketToSend> packet) {
    assert(pacing_bitrate_ > RtcDataRate::Zero());
    const int priority = GetPriorityForType(packet->packet_type);
    EnqueuePacketInternal(std::move(packet), priority);
}

void PacingController::SetIncludeOverhead() {
    include_overhead_ = true;
    packet_queue_.SetIncludeOverhead();
}

void PacingController::SetTransportOverhead(DataSize overhead_per_packet) {
    if (ignore_transport_overhead_) {
        return;
    }
    transport_overhead_per_packet_ = overhead_per_packet;
    packet_queue_.SetTransportOverhead(overhead_per_packet);
}

TimeDelta PacingController::ExpectedQueueTime() const {
    assert(pacing_bitrate_ > RtcDataRate::Zero());
    return TimeDelta::Millis(
        (QueueSizeData().bytes() * 8 * 1000) /
        pacing_bitrate_.bps());
}

size_t PacingController::QueueSizePackets() const {
    return packet_queue_.SizeInPackets();
}

DataSize PacingController::QueueSizeData() const {
    return packet_queue_.Size();
}

DataSize PacingController::CurrentBufferLevel() const {
    return std::max(media_debt_, padding_debt_);
}

std::optional<Timestamp> PacingController::FirstSentPacketTime() const {
    return first_sent_packet_time_;
}

Timestamp PacingController::OldestPacketEnqueueTime() const {
    return packet_queue_.OldestEnqueueTime();
}

void PacingController::EnqueuePacketInternal(std::unique_ptr<PacketToSend> packet, int priority) {
    prober_.OnIncomingPacket(DataSize::Bytes((int64_t)packet->packet->GetSize()));
    Timestamp now = CurrentTime();
    if (mode_ == ProcessMode::kDynamic && packet_queue_.Empty()) {
        // If queue is empty, we need to "fast-forward" the last process time,
        // so that we don't use passed time as budget for sending the first new
        // packet.
        Timestamp target_process_time = now;
        Timestamp next_send_time = NextSendTime();
        if (next_send_time.IsFinite()) {
            // There was already a valid planned send time, such as a keep-alive.
            // Use that as last process time only if it's prior to now.
            target_process_time = std::min(now, next_send_time);
        }

        TimeDelta elapsed_time = UpdateTimeAndGetElapsed(target_process_time);
        UpdateBudgetWithElapsedTime(elapsed_time);
        last_process_time_ = target_process_time;
    }
    packet_queue_.Push(priority, now, packet_counter_++, std::move(packet));
}

//距离上次process时间过了多少时间？
TimeDelta PacingController::UpdateTimeAndGetElapsed(Timestamp now) {
    // If no previous processing, or last process was "in the future" because of
    // early probe processing, then there is no elapsed time to add budget for.
    if (last_process_time_.IsMinusInfinity() || now < last_process_time_) {
        return TimeDelta::Zero();
    }
    assert(now >= last_process_time_);
    TimeDelta elapsed_time = now - last_process_time_;
    last_process_time_ = now;
    if (elapsed_time > kMaxElapsedTime) {
        std::cout << "Elapsed time (" << elapsed_time.ms()
            << " ms) longer than expected, limiting to "
            << kMaxElapsedTime.ms();
        elapsed_time = kMaxElapsedTime;
    }
    return elapsed_time;
}

bool PacingController::ShouldSendKeepalive(Timestamp now) const {
    if (send_padding_if_silent_ || paused_ || Congested() ||
        packet_counter_ == 0) {
        // We send a padding packet every 500 ms to ensure we won't get stuck in
        // congested state due to no feedback being received.
        TimeDelta elapsed_since_last_send = now - last_send_time_;
        if (elapsed_since_last_send >= kCongestedPacketInterval) {
            return true;
        }
    }
    return false;
}

Timestamp PacingController::NextSendTime() const {
    const Timestamp now = CurrentTime();

    if (paused_) {
        return last_send_time_ + kPausedProcessInterval;
    }

    // If probing is active, that always takes priority.
    //如果正在探测，则返回下个报文发送时间
    if (prober_.is_probing()) {
        Timestamp probe_time = prober_.NextProbeTime(now);
        // `probe_time` == PlusInfinity indicates no probe scheduled.
        if (probe_time != Timestamp::PlusInfinity() && !probing_send_failure_) {
            return probe_time;
        }
    }
    //周期性探测模式，5ms为周期
    if (mode_ == ProcessMode::kPeriodic) {
        // In periodic non-probing mode, we just have a fixed interval.
        return last_process_time_ + min_packet_limit_;
    }

    // In dynamic mode, figure out when the next packet should be sent,
    // given the current conditions.
    //拥塞的时候，等待
    if (Congested() || packet_counter_ == 0) {
        // We need to at least send keep-alive packets with some interval.
        return last_send_time_ + kCongestedPacketInterval;
    }

    // Check how long until we can send the next media packet.
    /*
    * 媒体发送码率大于 0，且媒体数据包优先级队列非空时，根据自上次处理媒体数据包以来，
    * 已经发送的媒体数据量和媒体发送码率计算已发送媒体数据预期的传输时间，
    * 并用该传输时间加上上次处理时间，作为期望的下次发送时间；
    */
    if (media_rate_ > RtcDataRate::Zero() && !packet_queue_.Empty()) {
        return std::min(last_send_time_ + kPausedProcessInterval,
            last_process_time_ + media_debt_ / media_rate_);
    }

    // If we _don't_ have pending packets, check how long until we have
    // bandwidth for padding packets. Both media and padding debts must
    // have been drained to do this.
    /*
    * 填充发送码率大于 0，且媒体数据包优先级队列为空时，根据自上次处理媒体数据包以来，
    * 已经发送的填充数据量和填充发送码率计算已发送填充数据预期的传输时间，并用该传输时间加上上次处理时间，
    * 作为期望的下次发送时间
    * media_debt_:预计还在传输中，没有传输完成的媒体数据量
    * padding_debt_:预计还在传输中，没有传输完成的填充数据量
    */
    if (padding_rate_ > RtcDataRate::Zero() && packet_queue_.Empty()) {
        TimeDelta drain_time =
            std::max(media_debt_ / media_rate_, padding_debt_ / padding_rate_);
        return std::min(last_send_time_ + kPausedProcessInterval,
            last_process_time_ + drain_time);
    }
    //需要静默时发送填充包，则将上次发送时间之后 500 ms 的时间点作为期望的下次发送时间
    if (send_padding_if_silent_) {
        return last_send_time_ + kPausedProcessInterval;
    }
    //将上次处理时间之后 500 ms 的时间点作为期望的下次发送时间
    return last_process_time_ + kPausedProcessInterval;
}

void PacingController::ProcessPackets() {
    Timestamp now = CurrentTime();
    Timestamp target_send_time = now;
    /*
     * 1.如果媒体数据包处理模式是 kDynamic，则检查期望的发送时间和当前时间的对比，当前时间比期望的发送时间提早的过多时，
     * 则仅更新 上次处理时间，并根据自 上次处理时间 以来的时长更新用于数据包发送调度和发送控制的参数，之后返回退出，否则继续执行
     */
    if (mode_ == ProcessMode::kDynamic) {
        target_send_time = NextSendTime(); //期望发送时间
        TimeDelta early_execute_margin =
            prober_.is_probing() ? kMaxEarlyProbeProcessing : TimeDelta::Zero();
        if (target_send_time.IsMinusInfinity()) {
            target_send_time = now;
        }
       
        else if (now < target_send_time - early_execute_margin) {
            // We are too early, but if queue is empty still allow draining some debt.
            // Probing is allowed to be sent up to kMinSleepTime early.
            TimeDelta elapsed_time = UpdateTimeAndGetElapsed(now);
            //根据距离上次process的时间elasped_time来更新预算
            UpdateBudgetWithElapsedTime(elapsed_time);
            return;
        }

        if (target_send_time < last_process_time_) {
            // After the last process call, at time X, the target send time
            // shifted to be earlier than X. This should normally not happen
            // but we want to make sure rounding errors or erratic behavior
            // of NextSendTime() does not cause issue. In particular, if the
            // buffer reduction of
            // rate * (target_send_time - previous_process_time)
            // in the main loop doesn't clean up the existing debt we may not
            // be able to send again. We don't want to check this reordering
            // there as it is the normal exit condtion when the buffer is
            // exhausted and there are packets in the queue.
            UpdateBudgetWithElapsedTime(last_process_time_ - target_send_time);
            target_send_time = last_process_time_;
        }
    }
    //2. 保存 上次处理时间，更新 上次处理时间，计算自 上次处理时间 以来的时长并保存
    Timestamp previous_process_time = last_process_time_;
    TimeDelta elapsed_time = UpdateTimeAndGetElapsed(now);
    //3. 处理连接保活。长时间没有发送过数据包时，会生成一些填充包并发送出去，以实现连接保活：
    if (ShouldSendKeepalive(now)) {
        // We can not send padding unless a normal packet has first been sent. If
        // we do, timestamps get messed up.
        if (packet_counter_ == 0) {
            last_send_time_ = now;
        }
        else {
            DataSize keepalive_data_sent = DataSize::Zero();
            std::vector<std::unique_ptr<PacketToSend>> keepalive_packets =
                packet_sender_->GeneratePadding(DataSize::Bytes(1));
            for (auto& packet : keepalive_packets) {
                keepalive_data_sent +=
                    DataSize::Bytes(packet->packet->GetSize() + /*packet->padding_size()*/ 0);
                packet_sender_->SendPacket(std::move(packet), PacedPacketInfo());
                /*
                * 每次发送数据包（不管是媒体数据包，还是填充包）之后，都会尝试获取 FEC 数据包，
                * 并将获得的 FEC 数据包放进媒体数据包优先级队列
                */
                for (auto& packet : packet_sender_->FetchFec()) {
                    EnqueuePacket(std::move(packet));
                }
            }
            OnPaddingSent(keepalive_data_sent);
        }
    }

    if (paused_) {
        return;
    }
    /*
    * 4.根据设置的发送码率和当前的发送情况更新媒体数据发送码率。获取媒体数据包优先级队列中数据量的大小，
    * 通过设置的数据包入队时长限制和媒体数据包优先级队列中数据包的平均入队时长，
    * 计算媒体数据包优先级队列中数据包的平均剩余时间，并计算所需的最小发送码率，
    * 并进一步计算新的目标发送码率。这里对目标发送码率的更新将影响后续发送的数据。
    */
    if (elapsed_time > TimeDelta::Zero()) {
        RtcDataRate target_rate = pacing_bitrate_;
        DataSize queue_size_data = packet_queue_.Size();
        if (queue_size_data > DataSize::Zero()) {
            // Assuming equal size packets and input/output rate, the average packet
            // has avg_time_left_ms left to get queue_size_bytes out of the queue, if
            // time constraint shall be met. Determine bitrate needed for that.
            packet_queue_.UpdateQueueTime(now);
            //大队列排干需要的最小码率
            if (drain_large_queues_) {
                TimeDelta avg_time_left =
                    std::max(TimeDelta::Millis(1),
                        queue_time_limit - packet_queue_.AverageQueueTime());
                RtcDataRate min_rate_needed = queue_size_data / avg_time_left;
                if (min_rate_needed > target_rate) {
                    target_rate = min_rate_needed;
                    std::cout << "bwe:large_pacing_queue pacing_rate_kbps="
                        << target_rate.kbps() << std::endl;
                }
            }
        }

        if (mode_ == ProcessMode::kPeriodic) {
            // In periodic processing mode, the IntevalBudget allows positive budget
            // up to (process interval duration) * (target rate), so we only need to
            // update it once before the packet sending loop.
            media_budget_.set_target_rate_kbps(target_rate.kbps());
            UpdateBudgetWithElapsedTime(elapsed_time);
        }
        else {
            media_rate_ = target_rate;
        }
    }
    /* 
    * 5. 正在执行码率探测时，计算获取码率探测器 webBitrateProber 建议发送的最小探测数据量。
    * 当媒体数据包优先级队列中的媒体数据包数据量小于这个建议的最小探测数据量时，可能会生成填充包并发送。
    */
    bool first_packet_in_probe = false;
    PacedPacketInfo pacing_info;
    DataSize recommended_probe_size = DataSize::Zero();
    bool is_probing = prober_.is_probing();
    if (is_probing) {
        // Probe timing is sensitive, and handled explicitly by BitrateProber, so
        // use actual send time rather than target.
        pacing_info = prober_.CurrentCluster(now).value_or(PacedPacketInfo());
        if (pacing_info.probe_cluster_id != PacedPacketInfo::kNotAProbe) {
            first_packet_in_probe = pacing_info.probe_cluster_bytes_sent == 0;
            recommended_probe_size = prober_.RecommendedMinProbeSize();
            assert(recommended_probe_size > DataSize::Zero());
        }
        else {
            // No valid probe cluster returned, probe might have timed out.
            is_probing = false;
        }
    }

    DataSize data_sent = DataSize::Zero();

    // The paused state is checked in the loop since it leaves the critical
    // section allowing the paused state to be changed from other code.

    //6. 发送媒体数据包。webPacingController::ProcessPackets() 用一个循环发送媒体数据包：
    while (!paused_) {
        if (first_packet_in_probe) {
            // If first packet in probe, insert a small padding packet so we have a
            // more reliable start window for the rate estimation.
            auto padding = packet_sender_->GeneratePadding(DataSize::Bytes(1));
            // If no RTP modules sending media are registered, we may not get a
            // padding packet back.
            if (!padding.empty()) {
                // Insert with high priority so larger media packets don't preempt it.
                EnqueuePacketInternal(std::move(padding[0]), kFirstPriority);
                // We should never get more than one padding packets with a requested
                // size of 1 byte.
                assert(padding.size() == 1u);
            }
            first_packet_in_probe = false;
        }
        /*
        * (1). 如果媒体数据包处理模式是 kDynamic，则检查期望的发送时间和 前一次数据包处理时间 的对比，
        * 当前者大于后者时，则根据两者的差值更新预计仍在传输中的数据量，以及 前一次数据包处理时间；
        */
        if (mode_ == ProcessMode::kDynamic &&
            previous_process_time < target_send_time) {
            // Reduce buffer levels with amount corresponding to time between last
            // process and target send time for the next packet.
            // If the process call is late, that may be the time between the optimal
            // send times for two packets we should already have sent.
            UpdateBudgetWithElapsedTime(target_send_time - previous_process_time);
            previous_process_time = target_send_time;
        }

        // Fetch the next packet, so long as queue is not empty or budget is not
        // exhausted.
        //(2). 从媒体数据包优先级队列中取一个数据包出来；
        std::unique_ptr<PacketToSend> rtp_packet =
            GetPendingPacket(pacing_info, target_send_time, now);
        /*
        * (3) 第 (2) 步中取出的数据包为空，但已经发送的媒体数据的量还没有达到码率探测器 webBitrateProber 建议发送的最小探测数据量，
        * 则创建一些填充数据包放入媒体数据包优先级队列，并继续下一轮处理；
        */
        if (rtp_packet == nullptr) {
            break; //是后来加上的，本来没有
            // No packet available to send, check if we should send padding.
            DataSize padding_to_add = PaddingToAdd(recommended_probe_size, data_sent);
            if (padding_to_add > DataSize::Zero()) {
                std::vector<std::unique_ptr<PacketToSend>> padding_packets =
                    packet_sender_->GeneratePadding(padding_to_add);
                if (padding_packets.empty()) {
                    // No padding packets were generated, quite send loop.
                    break;
                }
                //填充数据入队
                for (auto& packet : padding_packets) {
                    EnqueuePacket(std::move(packet));
                }
                // Continue loop to send the padding that was just added.
                continue;
            }

            // Can't fetch new packet and no padding to send, exit send loop.
            break;
        }

        assert(rtp_packet != nullptr);
        const PacketType packet_type = rtp_packet->packet_type;
        DataSize packet_size = DataSize::Bytes((int64_t)(rtp_packet->packet->GetSize() +
            /*rtp_packet->padding_size()*/ 0));

        if (include_overhead_) {
            packet_size += DataSize::Bytes(/*rtp_packet->headers_size())*/ 0) +
                transport_overhead_per_packet_;
        }
        //(4). 发送取出的媒体数据包；
        packet_sender_->SendPacket(std::move(rtp_packet), pacing_info);
        //(5)获取 FEC 数据包，并放入媒体数据包优先级队列；
        for (auto& packet : packet_sender_->FetchFec()) {
            EnqueuePacket(std::move(packet));
        }
        data_sent += packet_size;

        // Send done, update send/process time to the target send time.
        //(6)根据发送的数据包的数据量，更新预计仍在传输中的数据量等信息；
        OnPacketSent(packet_type, packet_size, target_send_time);

        // If we are currently probing, we need to stop the send loop when we have
        // reached the send target.
        /*
        * (7)如果是在码率探测期间，且发送的数据量超出码率探测器 webBitrateProber
        * 建议发送的最小探测数据量，则结束发送过程；
        */
        if (is_probing && data_sent >= recommended_probe_size) {
            break;
        }
        //(8)如果媒体数据包处理模式是 kDynamic，则更新目标发送时间。
        if (mode_ == ProcessMode::kDynamic) {
            // Update target send time in case that are more packets that we are late
            // in processing.
            Timestamp next_send_time = NextSendTime();
            if (next_send_time.IsMinusInfinity()) {
                target_send_time = now;
            }
            else {
                target_send_time = std::min(now, next_send_time);
            }
        }
    }

    last_process_time_ = std::max(last_process_time_, previous_process_time);
    //当处于码率探测阶段时，发送的数据包的数据量会通知到码率探测器 webBitrateProber
    if (is_probing) {
        probing_send_failure_ = data_sent == DataSize::Zero();
        if (!probing_send_failure_) {
            prober_.ProbeSent(CurrentTime(), data_sent);
        }
    }
}

DataSize PacingController::PaddingToAdd(DataSize recommended_probe_size,
    DataSize data_sent) const {
    if (!packet_queue_.Empty()) {
        // Actual payload available, no need to add padding.
        return DataSize::Zero();
    }

    if (Congested()) {
        // Don't add padding if congested, even if requested for probing.
        return DataSize::Zero();
    }

    if (packet_counter_ == 0) {
        // We can not send padding unless a normal packet has first been sent. If we
        // do, timestamps get messed up.
        return DataSize::Zero();
    }

    if (!recommended_probe_size.IsZero()) {
        if (recommended_probe_size > data_sent) {
            return recommended_probe_size - data_sent;
        }
        return DataSize::Zero();
    }

    if (mode_ == ProcessMode::kPeriodic) {
        return DataSize::Bytes(static_cast<int64_t>(padding_budget_.bytes_remaining()));
    }
    else if (padding_rate_ > RtcDataRate::Zero() &&
        padding_debt_ == DataSize::Zero()) {
        return padding_target_duration_ * padding_rate_;
    }
    return DataSize::Zero();
}

std::unique_ptr<PacketToSend> PacingController::GetPendingPacket(
    const PacedPacketInfo& pacing_info,
    Timestamp target_send_time,
    Timestamp now) {
    if (packet_queue_.Empty()) {
        return nullptr;
    }    
   
    bool is_probe = pacing_info.probe_cluster_id != PacedPacketInfo::kNotAProbe;
    if (!is_probe) {
        if (Congested()) {
            // Don't send anything if congested.
            return nullptr;
        }

        if (mode_ == ProcessMode::kPeriodic) {
            if (media_budget_.bytes_remaining() <= 0) {
                // Not enough budget.
                return nullptr;
            }
        }
        else {
            // Dynamic processing mode.
            if (now <= target_send_time) {
                // We allow sending slightly early if we think that we would actually
                // had been able to, had we been right on time - i.e. the current debt
                // is not more than would be reduced to zero at the target sent time.
                TimeDelta flush_time = media_debt_ / media_rate_;
                if (now + flush_time > target_send_time) {
                    return nullptr;
                }
            }
        }
    }
    //std::cout << "packet queue packets个数 = " << packet_queue_.SizeInPackets() << std::endl;
    return packet_queue_.Pop();
}

void PacingController::OnPacketSent(PacketType packet_type,
    DataSize packet_size,
    Timestamp send_time) {
    if (!first_sent_packet_time_) {
		first_sent_packet_time_ = send_time;
	}
	// Update media bytes sent.
	UpdateBudgetWithSentData(packet_size);

	last_send_time_ = send_time;
	last_process_time_ = send_time;
}

void PacingController::OnPaddingSent(DataSize data_sent) {
    if (data_sent > DataSize::Zero()) {
        UpdateBudgetWithSentData(data_sent);
    }
    Timestamp now = CurrentTime();
    last_send_time_ = now;
    last_process_time_ = now;
}

/*
* 对于 kDynamic 数据包处理模式，webPacingController 用 media_debt_ 和 padding_debt_ 分别记录预计还在传输中，
*没有传输完成的媒体数据量和填充数据量。随着时间的流逝，这两个值将以媒体数据发送码率和填充数据发送码率的速率递减。
*这两个值除以对应的发送码率，可用以获取预期的下次发送时间。UpdateBudgetWithElapsedTime() 根据自 上次处理时间 以来的时长更新这两个值。
*/
void PacingController::UpdateBudgetWithElapsedTime(TimeDelta delta) {
    if (mode_ == ProcessMode::kPeriodic) {
        delta = std::min(kMaxProcessingInterval, delta);
        media_budget_.IncreaseBudget(delta.ms());
        padding_budget_.IncreaseBudget(delta.ms());
    }
    else {
        media_debt_ -= std::min(media_debt_, media_rate_ * delta);
        padding_debt_ -= std::min(padding_debt_, padding_rate_ * delta);
    }
}

void PacingController::UpdateBudgetWithSentData(DataSize size) {
    outstanding_data_ += size;
    if (mode_ == ProcessMode::kPeriodic) {
        media_budget_.UseBudget(size.bytes());
        padding_budget_.UseBudget(size.bytes());
    }
    else {
        media_debt_ += size;
        media_debt_ = std::min(media_debt_, media_rate_ * kMaxDebtInTime);
        padding_debt_ += size;
        padding_debt_ = std::min(padding_debt_, padding_rate_ * kMaxDebtInTime);
    }
}

void PacingController::SetQueueTimeLimit(TimeDelta limit) {
    queue_time_limit = limit;
}


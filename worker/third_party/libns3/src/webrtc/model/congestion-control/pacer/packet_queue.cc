#include "packet_queue.h"
#include "ns3/common.h"
static constexpr DataSize kMaxLeadingSize = DataSize::Bytes(1400);

PacketQueue::QueuedPacket::QueuedPacket(const QueuedPacket& rhs) =
default;
PacketQueue::QueuedPacket::~QueuedPacket() = default;

PacketQueue::QueuedPacket::QueuedPacket(
    int priority,
    Timestamp enqueue_time,
    uint64_t enqueue_order,
    std::multiset<Timestamp>::iterator enqueue_time_it,
    std::unique_ptr<PacketToSend> packet)
    : priority_(priority),
    enqueue_time_(enqueue_time),
    enqueue_order_(enqueue_order),
    is_retransmission_(packet->packet_type ==
        PacketType::kRetransmission),
    enqueue_time_it_(enqueue_time_it),
    owned_packet_(packet.release()) {}

bool PacketQueue::QueuedPacket::operator<(
    const PacketQueue::QueuedPacket& other) const {
    if (priority_ != other.priority_)
        return priority_ > other.priority_;
    if (is_retransmission_ != other.is_retransmission_)
        return other.is_retransmission_;

    return enqueue_order_ > other.enqueue_order_;
}

int PacketQueue::QueuedPacket::Priority() const {
    return priority_;
}

PacketType PacketQueue::QueuedPacket::Type() const {
    return owned_packet_->packet_type;
}

uint32_t PacketQueue::QueuedPacket::Ssrc() const {
    //return owned_packet_->Ssrc();
    return 0;
}

Timestamp PacketQueue::QueuedPacket::EnqueueTime() const {
    return enqueue_time_;
}

bool PacketQueue::QueuedPacket::IsRetransmission() const {
    return Type() == PacketType::kRetransmission;
}

uint64_t PacketQueue::QueuedPacket::EnqueueOrder() const {
    return enqueue_order_;
}

PacketToSend* PacketQueue::QueuedPacket::RtpPacket() const {
    return owned_packet_;
}

void PacketQueue::QueuedPacket::UpdateEnqueueTimeIterator(
    std::multiset<Timestamp>::iterator it) {
    enqueue_time_it_ = it;
}

std::multiset<Timestamp>::iterator
PacketQueue::QueuedPacket::EnqueueTimeIterator() const {
    return enqueue_time_it_;
}

void PacketQueue::QueuedPacket::SubtractPauseTime(
    TimeDelta pause_time_sum) {
    enqueue_time_ -= pause_time_sum;
}

PacketQueue::PriorityPacketQueue::const_iterator
PacketQueue::PriorityPacketQueue::begin() const {
    return c.begin();
}

PacketQueue::PriorityPacketQueue::const_iterator
PacketQueue::PriorityPacketQueue::end() const {
    return c.end();
}

PacketQueue::PacketQueue(Timestamp start_time)
    : transport_overhead_per_packet_(DataSize::Zero()),
    time_last_updated_(start_time),
    paused_(false),
    size_packets_(0),
    size_(DataSize::Zero()),
    max_size_(kMaxLeadingSize),
    queue_time_sum_(TimeDelta::Zero()),
    pause_time_sum_(TimeDelta::Zero()),
    include_overhead_(false) {}

PacketQueue::~PacketQueue() {
    // Make sure to release any packets owned by raw pointer in QueuedPacket.
    while (!Empty()) {
        Pop();
    }
}

void PacketQueue::Push(int priority, Timestamp enqueue_time, uint64_t enqueue_order,
    std::unique_ptr<PacketToSend> packet) {

    if (size_packets_ == 0) {
        // Single packet fast-path.
        //对数据的封装，添加了优先级
        single_packet_queue_.emplace(
            QueuedPacket(priority, enqueue_time, enqueue_order,
                enqueue_times_.end(), std::move(packet)));
        UpdateQueueTime(enqueue_time);
        single_packet_queue_->SubtractPauseTime(pause_time_sum_);
        size_packets_ = 1;
        size_ += PacketSize(*single_packet_queue_);
    }
    else {
        // 如果single_packet_queue_有数据，先push到queue中
        MaybePromoteSinglePacketToNormalQueue();
        // 调用另外一个Push函数插入到队列中
        Push(QueuedPacket(priority, enqueue_time, enqueue_order,
            enqueue_times_.insert(enqueue_time), std::move(packet)));
    }
}

std::unique_ptr<PacketToSend> PacketQueue::Pop() {
    if (single_packet_queue_.has_value()) {        
        std::unique_ptr<PacketToSend> rtp_packet(
            single_packet_queue_->RtpPacket());
        single_packet_queue_.reset();
        queue_time_sum_ = TimeDelta::Zero();
        size_packets_ = 0;
        size_ = DataSize::Zero();
        return rtp_packet;
    }

    assert(!Empty());    
    const QueuedPacket& queued_packet = packet_queue_.top();
    // Calculate the total amount of time spent by this packet in the queue
    // while in a non-paused state. Note that the `pause_time_sum_ms_` was
    // subtracted from `packet.enqueue_time_ms` when the packet was pushed, and
    // by subtracting it now we effectively remove the time spent in in the
    // queue while in a paused state.
    TimeDelta time_in_non_paused_state =
        time_last_updated_ - queued_packet.EnqueueTime() - pause_time_sum_;
    queue_time_sum_ -= time_in_non_paused_state;

    assert(queued_packet.EnqueueTimeIterator() != enqueue_times_.end());
    enqueue_times_.erase(queued_packet.EnqueueTimeIterator());

    // Update `bytes` of this stream. The general idea is that the stream that
    // has sent the least amount of bytes should have the highest priority.
    // The problem with that is if streams send with different rates, in which
    // case a "budget" will be built up for the stream sending at the lower
    // rate. To avoid building a too large budget we limit `bytes` to be within
    // kMaxLeading bytes of the stream that has sent the most amount of bytes.
    DataSize packet_size = PacketSize(queued_packet);
    size_ -= packet_size;
    size_packets_ -= 1;
    assert(size_packets_ > 0 || queue_time_sum_ == TimeDelta::Zero());

    std::unique_ptr<PacketToSend> rtp_packet(queued_packet.RtpPacket());
    packet_queue_.pop();

    return rtp_packet;
}

bool PacketQueue::Empty() const {
    if (size_packets_ == 0) {
        assert(!single_packet_queue_.has_value() && packet_queue_.empty());
        return true;
    }
    assert(single_packet_queue_.has_value() || !packet_queue_.empty());
    return false;
}

size_t PacketQueue::SizeInPackets() const {
    return size_packets_;
}

DataSize PacketQueue::Size() const {
    return size_;
}


Timestamp PacketQueue::OldestEnqueueTime() const {
    if (single_packet_queue_.has_value()) {
        return single_packet_queue_->EnqueueTime();
    }

    if (Empty())
        return Timestamp::MinusInfinity();
    assert(!enqueue_times_.empty());
    return *enqueue_times_.begin();
}

void PacketQueue::UpdateQueueTime(Timestamp now) {
    assert(now >= time_last_updated_);
    if (now == time_last_updated_)
        return;

    TimeDelta delta = now - time_last_updated_;

    if (paused_) {
        pause_time_sum_ += delta;
    }
    else {
        queue_time_sum_ += TimeDelta::Micros((int64_t)(delta.us() * size_packets_));
    }

    time_last_updated_ = now;
}

void PacketQueue::SetPauseState(bool paused, Timestamp now) {
    if (paused_ == paused)
        return;
    UpdateQueueTime(now);
    paused_ = paused;
}

void PacketQueue::SetIncludeOverhead() {
    MaybePromoteSinglePacketToNormalQueue();
	include_overhead_ = true;
	// We need to update the size to reflect overhead for existing packets.

	for (const QueuedPacket& packet : packet_queue_) {
		size_ += DataSize::Bytes(static_cast<int64_t>(packet.RtpPacket()->headers_size())) +
			transport_overhead_per_packet_;
	}
    
}

void PacketQueue::SetTransportOverhead(DataSize overhead_per_packet) {
    MaybePromoteSinglePacketToNormalQueue();
	if (include_overhead_) {
		DataSize previous_overhead = transport_overhead_per_packet_;
		// We need to update the size to reflect overhead for existing packets.
		int packets = packet_queue_.size();
		size_ -= packets * previous_overhead;
		size_ += packets * overhead_per_packet;

	}
	transport_overhead_per_packet_ = overhead_per_packet;
}

TimeDelta PacketQueue::AverageQueueTime() const {
    if (Empty())
        return TimeDelta::Zero();
    return queue_time_sum_ / size_packets_;
}

void PacketQueue::Push(QueuedPacket packet) {
    if (packet.EnqueueTimeIterator() == enqueue_times_.end()) {
        // Promotion from single-packet queue. Just add to enqueue times.
        packet.UpdateEnqueueTimeIterator(
            enqueue_times_.insert(packet.EnqueueTime()));
    }
    else {
        // In order to figure out how much time a packet has spent in the queue
        // while not in a paused state, we subtract the total amount of time the
        // queue has been paused so far, and when the packet is popped we subtract
        // the total amount of time the queue has been paused at that moment. This
        // way we subtract the total amount of time the packet has spent in the
        // queue while in a paused state.
        UpdateQueueTime(packet.EnqueueTime());
        packet.SubtractPauseTime(pause_time_sum_);

        size_packets_ += 1;
        size_ += PacketSize(packet);
    }
    packet_queue_.push(packet);
}

DataSize PacketQueue::PacketSize(const QueuedPacket& packet) const {
    DataSize packet_size = DataSize::Bytes((int64_t)(packet.RtpPacket()->PayloadSize() +
        packet.RtpPacket()->padding_size()));
    if (include_overhead_) {
        packet_size += DataSize::Bytes(static_cast<int64_t>(packet.RtpPacket()->headers_size())) +
            transport_overhead_per_packet_;
    }
    return packet_size;
}

void PacketQueue::MaybePromoteSinglePacketToNormalQueue() {
    if (single_packet_queue_.has_value()) {
        Push(*single_packet_queue_);
        single_packet_queue_.reset();
    }
}


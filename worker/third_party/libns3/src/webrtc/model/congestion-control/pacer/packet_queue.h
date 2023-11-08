#pragma once
#include <memory>
#include <set>
#include <optional>
#include "ns3/timestamp.h"
#include "ns3/data_size.h"
#include <queue>
#include "ns3/common.h"

using namespace rtc;
class PacketQueue {
public:
	PacketQueue(Timestamp start_time);
	~PacketQueue();

	void Push(int priority,
		Timestamp enqueue_time,
		uint64_t enqueue_order,
		std::unique_ptr<PacketToSend> packet);

	std::unique_ptr<PacketToSend> Pop();

	bool Empty() const;
	size_t SizeInPackets() const;
	DataSize Size() const;

	Timestamp OldestEnqueueTime() const;
	TimeDelta AverageQueueTime() const;
	void UpdateQueueTime(Timestamp now);
	void SetPauseState(bool paused, Timestamp now);
	void SetIncludeOverhead();
	void SetTransportOverhead(DataSize overhead_per_packet);

private:
	struct QueuedPacket {
	public:
		QueuedPacket(int priority,
			Timestamp enqueue_time,
			uint64_t enqueue_order,
			std::multiset<Timestamp>::iterator enqueue_time_it,
			std::unique_ptr<PacketToSend> packet);
		QueuedPacket(const QueuedPacket& rhs);
		~QueuedPacket();

		bool operator<(const QueuedPacket& other) const;

		int Priority() const;
		PacketType Type() const;
		uint32_t Ssrc() const;
		Timestamp EnqueueTime() const;
		bool IsRetransmission() const;
		uint64_t EnqueueOrder() const;
		PacketToSend* RtpPacket() const;

		std::multiset<Timestamp>::iterator EnqueueTimeIterator() const;
		void UpdateEnqueueTimeIterator(std::multiset<Timestamp>::iterator it);
		void SubtractPauseTime(TimeDelta pause_time_sum);

	private:
		int priority_;
		Timestamp enqueue_time_; // Absolute time of pacer queue entry.
		uint64_t enqueue_order_;
		bool is_retransmission_; 
		std::multiset<Timestamp>::iterator enqueue_time_it_;
		// Raw pointer since priority_queue doesn't allow for moving
		// out of the container.
		PacketToSend* owned_packet_;
	};

	class PriorityPacketQueue : public std::priority_queue<QueuedPacket> {
	public:
		using const_iterator = container_type::const_iterator;
		const_iterator begin() const;
		const_iterator end() const;
	};
	PriorityPacketQueue packet_queue_;

	void Push(QueuedPacket packet);
	DataSize PacketSize(const QueuedPacket& packet) const;
	void MaybePromoteSinglePacketToNormalQueue();

	DataSize transport_overhead_per_packet_;

	Timestamp time_last_updated_;

	bool paused_;
	size_t size_packets_;
	DataSize size_;
	DataSize max_size_;
	TimeDelta queue_time_sum_;
	TimeDelta pause_time_sum_;

	// The enqueue time of every packet currently in the queue. Used to figure out
	// the age of the oldest packet in the queue.
	std::multiset<Timestamp> enqueue_times_;

	std::optional<QueuedPacket> single_packet_queue_;

	bool include_overhead_;

};
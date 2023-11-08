#pragma once
#include <optional>
#include "ns3/common.h"
#include "ns3/time_delta.h"
#include "ns3/data_rate.h"
#include "ns3/data_size.h"
#include "ns3/interval_budget.h"
#include "bitrate_prober.h"
#include "packet_queue.h"

using namespace rtc;
class PacketRouter;
class PacingController {
public:
    // Periodic mode uses the IntervalBudget class for tracking bitrate
    // budgets, and expected ProcessPackets() to be called a fixed rate,
    // e.g. every 5ms as implemented by PacedSender.
    // Dynamic mode allows for arbitrary time delta between calls to
    // ProcessPackets.
	enum class ProcessMode{kPeriodic, kDynamic};
	// Expected max pacer delay. If ExpectedQueueTime() is higher than
    // this value, the packet producers should wait (eg drop frames rather than
    // encoding them). Bitrate sent may temporarily exceed target set by
    // UpdateBitrate() so that this limit will be upheld.
	static const TimeDelta kMaxExpectedQueueLength;
	// Pacing-rate relative to our target send rate.
	// Multiplicative factor that is applied to the target bitrate to calculate
	// the number of bytes that can be transmitted per interval.
	// Increasing this factor will result in lower delays in cases of bitrate
	// overshoots from the encoder.
	static const float kDefaultPaceMultiplier;
	// If no media or paused, wake up at least every `kPausedProcessIntervalMs` in
	// order to send a keep-alive packet so we don't get stuck in a bad state due
	// to lack of feedback.
	static const TimeDelta kPausedProcessInterval;

	static const TimeDelta kMinSleepTime;

	PacingController(PacketRouter* packet_router, ProcessMode mode);
	~PacingController();

	void EnqueuePacket(std::unique_ptr<PacketToSend> packet);
	void CreateProbeCluster(RtcDataRate bitrate, int cluster_id);

	void Pause();  // Temporarily pause all sending.
	void Resume(); // Resume sending packets.
	bool IsPaused() const;

	void SetCongestionWindow(DataSize congestion_window_size);
	void UpdateOutstandingData(DataSize outstanding_data);

	void SetPacingRates(RtcDataRate pacing_rate, RtcDataRate padding_rate);
	RtcDataRate pacing_rate() const { return pacing_bitrate_; }

	void SetIncludeOverhead();
	void SetTransportOverhead(DataSize overhead_per_packet);

	// Returns the time when the oldest packet was queued.
	Timestamp OldestPacketEnqueueTime() const;

	// Number of packets in the pacer queue.
	size_t QueueSizePackets() const;
	//Totals size of packets in the pacer queue
	DataSize QueueSizeData() const;

	// Current buffer level, i.e. max of media and padding debt.
	DataSize CurrentBufferLevel() const;

	// Returns the time when the first packet was sent.
	std::optional<Timestamp> FirstSentPacketTime() const;

	// Returns the number of milliseconds it will take to send the current
	// packets in the queue, given the current size and bitrate, ignoring prio.
	TimeDelta ExpectedQueueTime() const;
	void SetQueueTimeLimit(TimeDelta limit);

	// Enable bitrate probing. Enabled by default, mostly here to simplify
	// testing. Must be called before any packets are being sent to have an
	// effect.
	void SetProbingEnabled(bool enabled);

	// Returns the next time we expect ProcessPackets() to be called.
	Timestamp NextSendTime() const;

	// Check queue of pending packets and send them or padding packets, if budget
	// is available.
	void ProcessPackets();

	bool Congested() const;
	bool IsProbing() const;

private:
	void EnqueuePacketInternal(std::unique_ptr<PacketToSend> packet, int priority);
	TimeDelta UpdateTimeAndGetElapsed(Timestamp now);
	bool ShouldSendKeepalive(Timestamp now) const;

	// Updates the number of bytes that can be sent for the next time interval.
	void UpdateBudgetWithElapsedTime(TimeDelta delta);
	void UpdateBudgetWithSentData(DataSize size);

	DataSize PaddingToAdd(DataSize recommended_probe_size, DataSize data_sent) const;

	std::unique_ptr<PacketToSend> GetPendingPacket(
		const PacedPacketInfo& pacing_info,
		Timestamp target_send_time,
		Timestamp now);
	
	void OnPacketSent(PacketType packet_type, DataSize packet_size, Timestamp send_time);
	void OnPaddingSent(DataSize padding_sent);

	Timestamp CurrentTime() const;
	const ProcessMode mode_;
	PacketRouter* const packet_sender_;

	const bool drain_large_queues_;
	const bool send_padding_if_silent_;
	const bool ignore_transport_overhead_;

	// In dynamic mode, indicates the target size when requesting padding,
  // expressed as a duration in order to adjust for varying padding rate.
	const TimeDelta padding_target_duration_;

	TimeDelta min_packet_limit_;

	DataSize transport_overhead_per_packet_;

	// TODO(webrtc:9716): Remove this when we are certain clocks are monotonic.
	// The last millisecond timestamp returned by `clock_`.
	mutable Timestamp last_timestamp_;
	bool paused_;

	// In dynamic mode, `media_budget_` and `padding_budget_` will be used to
  // track when packets can be sent.
  // In periodic mode, `media_debt_` and `padding_debt_` will be used together
  // with the target rates.

  // This is the media budget, keeping track of how many bits of media
  // we can pace out during the current interval.
	IntervalBudget media_budget_;
	// This is the padding budget, keeping track of how many bits of padding we're
	// allowed to send out during the current interval. This budget will be
	// utilized when there's no media to send.
	IntervalBudget padding_budget_;

	DataSize media_debt_;
	DataSize padding_debt_;
	RtcDataRate media_rate_;
	RtcDataRate padding_rate_;

	BitrateProber prober_;
	bool probing_send_failure_;

	RtcDataRate pacing_bitrate_;

	Timestamp last_process_time_; //上次调用process的时间
	Timestamp last_send_time_;
	std::optional<Timestamp> first_sent_packet_time_;

	PacketQueue packet_queue_;
	uint64_t packet_counter_;

	DataSize congestion_window_size_;
	DataSize outstanding_data_;

	TimeDelta queue_time_limit;
	bool account_for_audio_;
	bool include_overhead_;
};
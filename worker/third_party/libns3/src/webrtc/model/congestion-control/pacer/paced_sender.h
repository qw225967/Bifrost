#pragma once
//对外，交给pacing controller来进行具体的处理
#include <stdint.h>
#include <mutex>
#include "ns3/data_rate.h"
#include "packet_router.h"
#include "pacing_controller.h"

using namespace rtc;
class PacedSender {
public:
	// Expected max pacer delay in ms. If ExpectedQueueTime() is higher than
	// this value, the packet producers should wait (eg drop frames rather than
	// encoding them). Bitrate sent may temporarily exceed target set by
	// UpdateBitrate() so that this limit will be upheld.
	static const int64_t kMaxQueueLengthMs;
	// Pacing-rate relative to our target send rate.
	// Multiplicative factor that is applied to the target bitrate to calculate
	// the number of bytes that can be transmitted per interval.
	// Increasing this factor will result in lower delays in cases of bitrate
	// overshoots from the encoder.
	static const float kDefaultPaceMultiplier;
	PacedSender(PacketRouter* packet_router);
	~PacedSender();

	// Adds the packet to the queue and calls PacketRouter::SendPacket() when
	// it's time to send.
	void EnqueuePackets(std::vector<std::unique_ptr<PacketToSend>> packets);
	void EnqueuePacket(std::unique_ptr<PacketToSend> packet);
	void CreateProbeCluster(RtcDataRate bitrate, int cluster_id);

	// Temporarily pause all sending.
	void Pause();

	// Resume sending packets.
	void Resume();

	void SetCongestionWindow(DataSize congestion_window_size);
	void UpdateOutstandingData(DataSize outstanding_data);

	// Sets the pacing rates. Must be called once before packets can be sent.
	void SetPacingRates(RtcDataRate pacing_rate, RtcDataRate padding_rate);

	
	void SetIncludeOverhead();
	void SetTransportOverhead(DataSize overhead_per_packet);

	// Returns the time since the oldest queued packet was enqueued.
	TimeDelta OldestPacketWaitTime() const;

	DataSize QueueSizeData() const;

	// Returns the time when the first packet was sent;
	std::optional<Timestamp> FirstSentPacketTime() const;

	// Returns the number of milliseconds it will take to send the current
	// packets in the queue, given the current size and bitrate, ignoring prio.
	TimeDelta ExpectedQueueTime() const;

	void SetQueueTimeLimit(TimeDelta limit);

	// Below are methods specific to this implementation, such as things related
	// to module processing thread specifics or methods exposed for test.
	void Start();
private:
	// Returns the number of milliseconds until the module want a worker thread
	// to call Process.
	int64_t TimeUntilNextProcess();
	// Called when the prober is associated with a process thread.	
	void Process();
	// In dynamic process mode, refreshes the next process time.
	void MaybeWakupProcessThread();
	mutable std::mutex mutex_;
	const PacingController::ProcessMode process_mode_;
	PacingController pacing_controller_;	
	//std::thread process_thread_;
};
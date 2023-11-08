#include "paced_sender.h"
#include <assert.h>
#include <thread>
#include "ns3/simulator.h"
const int64_t PacedSender::kMaxQueueLengthMs = 2000;
const float PacedSender::kDefaultPaceMultiplier = 2.5f;

//对于webrtc来说，一个线程应该是注册多个模块，这样一个线程才不会更多的浪费
PacedSender::PacedSender(PacketRouter* packet_router)
	:process_mode_(PacingController::ProcessMode::kPeriodic),
	pacing_controller_(packet_router, process_mode_)	 {    
}

PacedSender::~PacedSender() {}

void PacedSender::CreateProbeCluster(RtcDataRate bitrate, int cluster_id) {
	return pacing_controller_.CreateProbeCluster(bitrate, cluster_id);
}

void PacedSender::Pause() {	
	pacing_controller_.Pause();
}

void PacedSender::Resume() {	
	pacing_controller_.Resume();
}

void PacedSender::SetCongestionWindow(DataSize congestion_window_size) {
	{		
		pacing_controller_.SetCongestionWindow(congestion_window_size);
	}
	MaybeWakupProcessThread();
}

void PacedSender::UpdateOutstandingData(DataSize outstanding_data) {
    {        
        pacing_controller_.UpdateOutstandingData(outstanding_data);
    }
    MaybeWakupProcessThread();
}

void PacedSender::SetPacingRates(RtcDataRate pacing_rate, RtcDataRate padding_rate) {
    {        
        pacing_controller_.SetPacingRates(pacing_rate * kDefaultPaceMultiplier, padding_rate);
    }
    MaybeWakupProcessThread();
}

void PacedSender::EnqueuePackets(
    std::vector<std::unique_ptr<PacketToSend>> packets) {
        {            
            for (auto& packet : packets) {               
                pacing_controller_.EnqueuePacket(std::move(packet));
            }
        }
        MaybeWakupProcessThread();
}

void PacedSender::EnqueuePacket(std::unique_ptr<PacketToSend> packet) {    
    pacing_controller_.EnqueuePacket(std::move(packet));
}

void PacedSender::SetIncludeOverhead() {    
    pacing_controller_.SetIncludeOverhead();
}

void PacedSender::SetTransportOverhead(DataSize overhead_per_packet) {    
    pacing_controller_.SetTransportOverhead(overhead_per_packet);
}

TimeDelta PacedSender::ExpectedQueueTime() const {    
    return pacing_controller_.ExpectedQueueTime();
}

DataSize PacedSender::QueueSizeData() const {    
    return pacing_controller_.QueueSizeData();
}

std::optional<Timestamp> PacedSender::FirstSentPacketTime() const {    
    return pacing_controller_.FirstSentPacketTime();
}

TimeDelta PacedSender::OldestPacketWaitTime() const {    
    Timestamp oldest_packet = pacing_controller_.OldestPacketEnqueueTime();
    if (oldest_packet.IsInfinite())
        return TimeDelta::Zero();

    // (webrtc:9716): The clock is not always monotonic.
    Timestamp current = Timestamp::Millis(Simulator::Now().GetMilliSeconds());
    if (current < oldest_packet)
        return TimeDelta::Zero();
    return current - oldest_packet;
}

int64_t PacedSender::TimeUntilNextProcess() {    
    Timestamp next_send_time = pacing_controller_.NextSendTime();
    TimeDelta sleep_time =
        std::max(TimeDelta::Zero(), next_send_time - Timestamp::Millis(Simulator::Now().GetMilliSeconds()));
    if (process_mode_ == PacingController::ProcessMode::kDynamic) {
        return std::max(sleep_time, PacingController::kMinSleepTime).ms();
    }
    return sleep_time.ms();
}

void PacedSender::Process() {    
    pacing_controller_.ProcessPackets();
    int64_t time_interval = TimeUntilNextProcess();
    Simulator::Schedule(MilliSeconds(time_interval), &PacedSender::Process, this); 
}


void PacedSender::MaybeWakupProcessThread() {
    // Tell the process thread to call our TimeUntilNextProcess() method to get
    // a new time for when to call Process().
    
}

void PacedSender::SetQueueTimeLimit(TimeDelta limit) {
    {       
        pacing_controller_.SetQueueTimeLimit(limit);
    }
    MaybeWakupProcessThread();
}

void PacedSender::Start() {    
    int64_t time_interval = TimeUntilNextProcess();
    Simulator::Schedule(MilliSeconds(time_interval), &PacedSender::Process, this);       
}


#define MS_CLASS "RTC::TransportCongestionControlServer"
// #define MS_LOG_DEV_LEVEL 3

#include "bifrost/bifrost_send_algorithm/tcc_server.h"

#include <iterator>  // std::ostream_iterator
#include <sstream>   // std::ostringstream

#include "uv_loop.h"

namespace bifrost {
/* Static. */

static constexpr uint64_t TransportCcFeedbackSendInterval{100u};  // In ms.
static constexpr uint64_t LimitationRembInterval{1500u};          // In ms.
static constexpr uint8_t UnlimitedRembNumPackets{4u};
static constexpr size_t PacketLossHistogramLength{24};

/* Instance methods. */

TransportCongestionControlServer::TransportCongestionControlServer(
    TransportCongestionControlServer::Observer* observer,
    size_t maxRtcpPacketLen, UvLoop** uv_loop)
    : observer_(observer),
      maxRtcpPacketLen(maxRtcpPacketLen),
      uv_loop_(*uv_loop) {
  this->transportCcFeedbackPacket =
      std::make_shared<FeedbackRtpTransportPacket>(0u, 0u);

  this->quicFeedbackPacket = std::make_shared<QuicAckFeedbackPacket>(0u, 0u);

  // Set initial packet count.
  this->transportCcFeedbackPacket->SetFeedbackPacketCount(
      this->transportCcFeedbackPacketCount);

  // Create the feedback send periodic timer.
  this->transportCcFeedbackSendPeriodicTimer =
      new UvTimer(this, this->uv_loop_->get_loop().get());

  this->transportCcFeedbackSendPeriodicTimer->Start(
      TransportCcFeedbackSendInterval, TransportCcFeedbackSendInterval);
}

TransportCongestionControlServer::~TransportCongestionControlServer() {
  delete this->transportCcFeedbackSendPeriodicTimer;
  this->transportCcFeedbackSendPeriodicTimer = nullptr;

  // Delete REMB server.
  delete this->rembServer;
  this->rembServer = nullptr;

  packet_recv_time_map_.clear();
}

void TransportCongestionControlServer::QuicCountIncomingPacket(
    uint64_t nowMs, const RtpPacket* packet) {

  uint16_t wideSeqNumber;
  if (!packet->ReadTransportWideCc01(wideSeqNumber)) return;

  this->quicFeedbackPacket->SetSenderSsrc(0u);
  this->quicFeedbackPacket->SetMediaSsrc(this->transportCcFeedbackMediaSsrc);

  RecvPacketInfo temp;
  temp.recv_time = nowMs;
  temp.sequence = wideSeqNumber;
  temp.recv_bytes = packet->GetSize();
  this->packet_recv_time_map_[temp.sequence] = temp;
}

void TransportCongestionControlServer::SendQuicAckFeedback() {
  auto nowMs = this->uv_loop_->get_time_ms_int64();

  auto it = this->packet_recv_time_map_.begin();
  while (it != this->packet_recv_time_map_.end()) {
    auto feedbackItem = new QuicAckFeedbackItem(it->second.sequence,
                                                nowMs - it->second.recv_time,
                                                it->second.recv_bytes);
    this->quicFeedbackPacket->AddItem(feedbackItem);

    if (this->quicFeedbackPacket->GetSize() > BufferSize) {
      this->quicFeedbackPacket->Serialize(Buffer);

      this->observer_->OnTransportCongestionControlServerSendRtcpPacket(
          this, this->quicFeedbackPacket);

      this->quicFeedbackPacket.reset();
      this->quicFeedbackPacket = std::make_shared<QuicAckFeedbackPacket>(
          this->transportCcFeedbackSenderSsrc,
          this->transportCcFeedbackMediaSsrc);
    }

    it = this->packet_recv_time_map_.erase(it);
  }

  this->quicFeedbackPacket->Serialize(Buffer);

  this->observer_->OnTransportCongestionControlServerSendRtcpPacket(
      this, this->quicFeedbackPacket);

  this->quicFeedbackPacket.reset();

  this->quicFeedbackPacket = std::make_shared<QuicAckFeedbackPacket>(
      this->transportCcFeedbackSenderSsrc, this->transportCcFeedbackMediaSsrc);
}

void TransportCongestionControlServer::IncomingPacket(uint64_t nowMs,
                                                      const RtpPacket* packet) {
  uint16_t wideSeqNumber;

  if (!packet->ReadTransportWideCc01(wideSeqNumber)) return;

  // Update the RTCP media SSRC of the ongoing Transport-CC Feedback packet.
  this->transportCcFeedbackSenderSsrc = 0u;
  this->transportCcFeedbackMediaSsrc = packet->GetSsrc();

  this->transportCcFeedbackPacket->SetSenderSsrc(0u);
  this->transportCcFeedbackPacket->SetMediaSsrc(
      this->transportCcFeedbackMediaSsrc);

  // Provide the feedback packet with the RTP packet info. If it fails,
  // send current feedback and add the packet info to a new one.
  if (this->transportCcFeedbackPacket == nullptr) return;
  if (this->quicFeedbackPacket == nullptr) return;

  auto result = this->transportCcFeedbackPacket->AddPacket(
      wideSeqNumber, nowMs, this->maxRtcpPacketLen);

  switch (result) {
    case FeedbackRtpTransportPacket::AddPacketResult::SUCCESS: {
      // If the feedback packet is full, send it now.
      if (this->transportCcFeedbackPacket->IsFull()) {
        std::cout << "[tcc server] transport-cc feedback packet is full, "
                     "sending feedback now"
                  << std::endl;

        SendTransportCcFeedback();
      }

      break;
    }

    case FeedbackRtpTransportPacket::AddPacketResult::MAX_SIZE_EXCEEDED: {
      // Send ongoing feedback packet and add the new packet info to the
      // regenerated one.
      SendTransportCcFeedback();

      this->transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs,
                                                 this->maxRtcpPacketLen);

      break;
    }

    case FeedbackRtpTransportPacket::AddPacketResult::FATAL: {
      // Create a new feedback packet.
      this->transportCcFeedbackPacket =
          std::make_shared<FeedbackRtpTransportPacket>(
              this->transportCcFeedbackSenderSsrc,
              this->transportCcFeedbackMediaSsrc);

      // Use current packet count.
      // NOTE: Do not increment it since the previous ongoing feedback
      // packet was not sent.
      this->transportCcFeedbackPacket->SetFeedbackPacketCount(
          this->transportCcFeedbackPacketCount);

      break;
    }
  }

  MaySendLimitationRembFeedback();
}

void TransportCongestionControlServer::SetMaxIncomingBitrate(uint32_t bitrate) {
  auto previousMaxIncomingBitrate = this->maxIncomingBitrate;

  this->maxIncomingBitrate = bitrate;

  if (previousMaxIncomingBitrate != 0u && this->maxIncomingBitrate == 0u) {
    // This is to ensure that we send N REMB packets with bitrate 0 (unlimited).
    this->unlimitedRembCounter = UnlimitedRembNumPackets;

    MaySendLimitationRembFeedback();
  }
}

inline void TransportCongestionControlServer::SendTransportCcFeedback() {
  if (!this->transportCcFeedbackPacket->IsSerializable()) return;

  auto latestWideSeqNumber =
      this->transportCcFeedbackPacket->GetLatestSequenceNumber();
  auto latestTimestamp = this->transportCcFeedbackPacket->GetLatestTimestamp();

//  std::cout << "SendTransportCcFeedback latestWideSeqNumber:"
//            << latestWideSeqNumber << ", latestTimestamp:" << latestTimestamp
//            << ", base seq:"
//            << this->transportCcFeedbackPacket->GetBaseSequenceNumber()
//            << ", feedback count:"
//            << (uint32_t)this->transportCcFeedbackPacket->GetFeedbackPacketCount()
//            << std::endl;

  // Notify the listener.
  this->observer_->OnTransportCongestionControlServerSendRtcpPacket(
      this, this->transportCcFeedbackPacket);

  // Update packet loss history.
  size_t expected_packets =
      this->transportCcFeedbackPacket->GetPacketStatusCount();

  size_t lost_packets = 0;
  for (const auto& result :
       this->transportCcFeedbackPacket->GetPacketResults()) {
    if (!result.received) lost_packets += 1;
  }

  this->UpdatePacketLoss(static_cast<double>(lost_packets) / expected_packets);

  // Create a new feedback packet.
  this->transportCcFeedbackPacket =
      std::make_shared<FeedbackRtpTransportPacket>(
          this->transportCcFeedbackSenderSsrc,
          this->transportCcFeedbackMediaSsrc);

  // Increment packet count.
  this->transportCcFeedbackPacket->SetFeedbackPacketCount(
      ++this->transportCcFeedbackPacketCount);

  // Pass the latest packet info (if any) as pre base for the new feedback
  // packet.
  if (latestTimestamp > 0u) {
    this->transportCcFeedbackPacket->AddPacket(
        latestWideSeqNumber, latestTimestamp, this->maxRtcpPacketLen);
  }
}

inline void TransportCongestionControlServer::MaySendLimitationRembFeedback() {}

void TransportCongestionControlServer::UpdatePacketLoss(double packetLoss) {
  // Add the score into the histogram.
  if (this->packetLossHistory.size() == PacketLossHistogramLength)
    this->packetLossHistory.pop_front();

  this->packetLossHistory.push_back(packetLoss);

  // Calculate a weighted average
  size_t weight{0};
  size_t samples{0};
  double totalPacketLoss{0};

  for (auto packetLossEntry : this->packetLossHistory) {
    weight++;
    samples += weight;
    totalPacketLoss += weight * packetLossEntry;
  }

  // clang-tidy "thinks" that this can lead to division by zero but we are
  // smarter.
  // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
  this->packetLoss = totalPacketLoss / samples;
}

inline void TransportCongestionControlServer::OnRembServerAvailableBitrate(
    const webrtc::RemoteBitrateEstimator* /*rembServer*/,
    const std::vector<uint32_t>& ssrcs, uint32_t availableBitrate) {}

inline void TransportCongestionControlServer::OnTimer(UvTimer* timer) {
  if (timer == this->transportCcFeedbackSendPeriodicTimer) {
    SendTransportCcFeedback();
    SendQuicAckFeedback();
  }
}
}  // namespace bifrost

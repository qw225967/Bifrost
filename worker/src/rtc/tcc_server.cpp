#define MS_CLASS "RTC::TransportCongestionControlServer"
// #define MS_LOG_DEV_LEVEL 3

#include "tcc_server.h"

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
    TransportCongestionControlServer::Listener* listener,
    size_t maxRtcpPacketLen, UvLoop* uv_loop)
    : listener(listener), maxRtcpPacketLen(maxRtcpPacketLen) {
  this->transportCcFeedbackPacket =
      std::make_unique<FeedbackRtpTransportPacket>(0u, 0u);

  // Set initial packet count.
  this->transportCcFeedbackPacket->SetFeedbackPacketCount(
      this->transportCcFeedbackPacketCount);

  // Create the feedback send periodic timer.
  this->transportCcFeedbackSendPeriodicTimer =
      new UvTimer(this, uv_loop->get_loop().get());
}

TransportCongestionControlServer::~TransportCongestionControlServer() {
  delete this->transportCcFeedbackSendPeriodicTimer;
  this->transportCcFeedbackSendPeriodicTimer = nullptr;

  // Delete REMB server.
  delete this->rembServer;
  this->rembServer = nullptr;
}

void TransportCongestionControlServer::IncomingPacket(uint64_t nowMs,
                                                      const RtpPacket* packet) {
  uint16_t wideSeqNumber;

  if (!packet->ReadTransportWideCc01(wideSeqNumber)) break;

  // Update the RTCP media SSRC of the ongoing Transport-CC Feedback packet.
  this->transportCcFeedbackSenderSsrc = 0u;
  this->transportCcFeedbackMediaSsrc = packet->GetSsrc();

  this->transportCcFeedbackPacket->SetSenderSsrc(0u);
  this->transportCcFeedbackPacket->SetMediaSsrc(
      this->transportCcFeedbackMediaSsrc);

  // Provide the feedback packet with the RTP packet info. If it fails,
  // send current feedback and add the packet info to a new one.
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
          std::make_unique<FeedbackRtpTransportPacket>(
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

  // Notify the listener.
  this->listener->OnTransportCongestionControlServerSendRtcpPacket(
      this, this->transportCcFeedbackPacket.get());

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
      std::make_unique<FeedbackRtpTransportPacket>(
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
  if (timer == this->transportCcFeedbackSendPeriodicTimer)
    SendTransportCcFeedback();
}
}  // namespace bifrost
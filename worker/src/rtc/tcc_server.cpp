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
    TransportCongestionControlServer::Observer* observer,
    size_t maxRtcpPacketLen, UvLoop** uv_loop)
    : observer_(observer),
      maxRtcpPacketLen(maxRtcpPacketLen),
      uv_loop_(*uv_loop) {
  this->transportCcFeedbackPacket =
      std::make_shared<FeedbackRtpTransportPacket>(0u, 0u);

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
}

void TransportCongestionControlServer::IncomingPacket(uint64_t nowMs,
                                                      const RtpPacket* packet) {
  uint16_t wideSeqNumber;

  std::cout << "IncomingPacket packet" << std::endl;

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

  auto result = this->transportCcFeedbackPacket->AddPacket(
      wideSeqNumber, nowMs, this->maxRtcpPacketLen);

  std::cout << "wideSeqNumber:" << wideSeqNumber << ", last seq:"
            << this->transportCcFeedbackPacket->GetLatestSequenceNumber()
            << std::endl;

  switch (result) {
    case FeedbackRtpTransportPacket::AddPacketResult::SUCCESS: {
      std::cout << "IncomingPacket SUCCESS" << std::endl;
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
      std::cout << "IncomingPacket MAX_SIZE_EXCEEDED" << std::endl;
      // Send ongoing feedback packet and add the new packet info to the
      // regenerated one.
      SendTransportCcFeedback();

      this->transportCcFeedbackPacket->AddPacket(wideSeqNumber, nowMs,
                                                 this->maxRtcpPacketLen);

      break;
    }

    case FeedbackRtpTransportPacket::AddPacketResult::FATAL: {
      std::cout << "IncomingPacket FATAL" << std::endl;
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

  std::cout << "OnTimer SendTransportCcFeedback 1" << std::endl;
  auto latestWideSeqNumber =
      this->transportCcFeedbackPacket->GetLatestSequenceNumber();
  auto latestTimestamp = this->transportCcFeedbackPacket->GetLatestTimestamp();
  std::cout << "OnTimer SendTransportCcFeedback 2" << std::endl;
  // Notify the listener.
  this->observer_->OnTransportCongestionControlServerSendRtcpPacket(
      this, this->transportCcFeedbackPacket);
  std::cout << "OnTimer SendTransportCcFeedback 3" << std::endl;
  // Update packet loss history.
  size_t expected_packets =
      this->transportCcFeedbackPacket->GetPacketStatusCount();
  std::cout << "OnTimer SendTransportCcFeedback 4" << std::endl;
  size_t lost_packets = 0;
  for (const auto& result :
       this->transportCcFeedbackPacket->GetPacketResults()) {
    if (!result.received) lost_packets += 1;
  }
  std::cout << "OnTimer SendTransportCcFeedback 5" << std::endl;
  this->UpdatePacketLoss(static_cast<double>(lost_packets) / expected_packets);

  std::cout << "OnTimer SendTransportCcFeedback 6" << std::endl;
  // Create a new feedback packet.
  this->transportCcFeedbackPacket =
      std::make_shared<FeedbackRtpTransportPacket>(
          this->transportCcFeedbackSenderSsrc,
          this->transportCcFeedbackMediaSsrc);

  std::cout << "OnTimer SendTransportCcFeedback 7" << std::endl;
  // Increment packet count.
  this->transportCcFeedbackPacket->SetFeedbackPacketCount(
      ++this->transportCcFeedbackPacketCount);

  std::cout << "OnTimer SendTransportCcFeedback 8" << std::endl;
  // Pass the latest packet info (if any) as pre base for the new feedback
  // packet.
  if (latestTimestamp > 0u) {
    this->transportCcFeedbackPacket->AddPacket(
        latestWideSeqNumber, latestTimestamp, this->maxRtcpPacketLen);
  }
  std::cout << "OnTimer SendTransportCcFeedback 9" << std::endl;
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
    std::cout << "OnTimer TransportCongestionControlServer" << std::endl;
    SendTransportCcFeedback();
  }
}
}  // namespace bifrost

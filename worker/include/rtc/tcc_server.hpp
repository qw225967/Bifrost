#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include <libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h>
#include <deque>

#include "rtp_packet.h"
#include "common.hpp"
#include "uv_timer.h"

namespace bifrost {
class TransportCongestionControlServer
    : public webrtc::RemoteBitrateEstimator::Listener,
      public UvTimer::Listener {
 public:
  class Listener {
   public:
    virtual ~Listener() = default;

   public:
    virtual void OnTransportCongestionControlServerSendRtcpPacket(
        TransportCongestionControlServer* tccServer,
        RTCP::Packet* packet) = 0;
  };

 public:
  TransportCongestionControlServer(
      TransportCongestionControlServer::Listener* listener,
      size_t maxRtcpPacketLen);
  virtual ~TransportCongestionControlServer();

 public:
  void TransportConnected();
  void TransportDisconnected();
  uint32_t GetAvailableBitrate() const {
    switch (this->bweType) {
      case RTC::BweType::REMB:
        return this->rembServer->GetAvailableBitrate();

      default:
        return 0u;
    }
  }
  double GetPacketLoss() const;
  void IncomingPacket(uint64_t nowMs, const RTC::RtpPacket* packet);
  void SetMaxIncomingBitrate(uint32_t bitrate);

 private:
  void SendTransportCcFeedback();
  void MaySendLimitationRembFeedback();
  void UpdatePacketLoss(double packetLoss);

  /* Pure virtual methods inherited from
   * webrtc::RemoteBitrateEstimator::Listener. */
 public:
  void OnRembServerAvailableBitrate(
      const webrtc::RemoteBitrateEstimator* remoteBitrateEstimator,
      const std::vector<uint32_t>& ssrcs, uint32_t availableBitrate) override;

  /* Pure virtual methods inherited from Timer::Listener. */
 public:
  void OnTimer(Timer* timer) override;

 private:
  // Passed by argument.
  Listener* listener{nullptr};
  // Allocated by this.
  Timer* transportCcFeedbackSendPeriodicTimer{nullptr};
  std::unique_ptr<RTC::RTCP::FeedbackRtpTransportPacket>
      transportCcFeedbackPacket;
  webrtc::RemoteBitrateEstimatorAbsSendTime* rembServer{nullptr};

  // Others.
  size_t maxRtcpPacketLen{0u};
  uint8_t transportCcFeedbackPacketCount{0u};
  uint32_t transportCcFeedbackSenderSsrc{0u};
  uint32_t transportCcFeedbackMediaSsrc{0u};
  uint32_t maxIncomingBitrate{0u};
  uint64_t limitationRembSentAtMs{0u};
  uint8_t unlimitedRembCounter{0u};
  std::deque<double> packetLossHistory;
  double packetLoss{0};
};
}  // namespace bifrost

#endif

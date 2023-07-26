#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_SERVER_HPP

#include <modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h>

#include <deque>
#include <map>

#include "common.h"
#include "rtcp_feedback.h"
#include "rtcp_packet.h"
#include "rtcp_tcc.h"
#include "rtp_packet.h"
#include "rtcp_quic_feedback.h"
#include "uv_timer.h"

namespace bifrost {
class TransportCongestionControlServer
    : public webrtc::RemoteBitrateEstimator::Listener,
      public UvTimer::Listener {
 private:
  struct RecvPacketInfo{
    uint16_t sequence;
    uint16_t recv_time;
    uint32_t recv_bytes;
  };
 public:
  class Observer {
   public:
    virtual ~Observer() = default;

   public:
    virtual void OnTransportCongestionControlServerSendRtcpPacket(
        TransportCongestionControlServer* tccServer, RtcpPacketPtr packet) = 0;
  };

 public:
  TransportCongestionControlServer(
      TransportCongestionControlServer::Observer* observer,
      size_t maxRtcpPacketLen, UvLoop** uv_loop);
  virtual ~TransportCongestionControlServer();

 public:
  uint32_t get_available_bitrate() const { return 0; }
  double get_packet_loss() const { this->packetLoss; };
  void IncomingPacket(uint64_t nowMs, const RtpPacket* packet);
  void QuicCountIncomingPacket(uint64_t nowMs, const RtpPacket* packet);
  void SetMaxIncomingBitrate(uint32_t bitrate);

 private:
  void SendQuicAckFeedback();
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
  void OnTimer(UvTimer* timer) override;

 private:
  // uv loop
  UvLoop* uv_loop_;

  // Passed by argument.
  Observer* observer_{nullptr};
  // Allocated by this.
  UvTimer* transportCcFeedbackSendPeriodicTimer{nullptr};
  std::shared_ptr<FeedbackRtpTransportPacket> transportCcFeedbackPacket;
  std::shared_ptr<QuicAckFeedbackPacket> quicFeedbackPacket;
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

  std::map<uint16_t, RecvPacketInfo> packet_recv_time_map_;
};
}  // namespace bifrost

#endif

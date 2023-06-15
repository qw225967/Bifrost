#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP

#include <api/transport/goog_cc_factory.h>
#include <api/transport/network_types.h>
#include <call/rtp_transport_controller_send.h>
#include <modules/pacing/packet_router.h>
#include <deque>

#include "rtp_packet.h"
#include "uv_timer.h"
#include "common.h"

namespace bifrost {
class TransportCongestionControlClient
    : public webrtc::PacketRouter,
      public webrtc::TargetTransferRateObserver,
      public UvTimer::Listener {
 public:
  struct Bitrates {
    uint32_t desiredBitrate{0u};
    uint32_t effectiveDesiredBitrate{0u};
    uint32_t minBitrate{0u};
    uint32_t maxBitrate{0u};
    uint32_t startBitrate{0u};
    uint32_t maxPaddingBitrate{0u};
    uint32_t availableBitrate{0u};
  };

 public:
  class Listener {
   public:
    virtual ~Listener() = default;

   public:
    virtual void OnTransportCongestionControlClientBitrates(
        TransportCongestionControlClient* tccClient,
        TransportCongestionControlClient::Bitrates& bitrates) = 0;
    virtual void OnTransportCongestionControlClientSendRtpPacket(
        TransportCongestionControlClient* tccClient,
        RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo) = 0;
  };

 public:
  TransportCongestionControlClient(
      TransportCongestionControlClient::Listener* listener,
      uint32_t initialAvailableBitrate, uint32_t maxOutgoingBitrate,
      UvLoop* uv_loop);
  virtual ~TransportCongestionControlClient();

 public:
  void TransportConnected();
  void TransportDisconnected();
  void InsertPacket(webrtc::RtpPacketSendInfo& packetInfo);
  webrtc::PacedPacketInfo GetPacingInfo();
  void PacketSent(webrtc::RtpPacketSendInfo& packetInfo, int64_t nowMs);
  void ReceiveEstimatedBitrate(uint32_t bitrate);

  void ReceiveRtcpTransportFeedback(
      const RTC::RTCP::FeedbackRtpTransportPacket* feedback);
  void SetDesiredBitrate(uint32_t desiredBitrate, bool force);
  void SetMaxOutgoingBitrate(uint32_t maxBitrate);
  const Bitrates& GetBitrates() const { return this->bitrates; }
  uint32_t GetAvailableBitrate() const;
  double GetPacketLoss() const;
  void RescheduleNextAvailableBitrateEvent();

 private:
  void MayEmitAvailableBitrateEvent(uint32_t previousAvailableBitrate);
  void UpdatePacketLoss(double packetLoss);
  void ApplyBitrateUpdates();

  void InitializeController();
  void DestroyController();

  // jmillan: missing.
  // void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

  /* Pure virtual methods inherited from webrtc::TargetTransferRateObserver. */
 public:
  void OnTargetTransferRate(
      webrtc::TargetTransferRate targetTransferRate) override;

  /* Pure virtual methods inherited from webrtc::PacketRouter. */
 public:
  void SendPacket(RtpPacket* packet,
                  const webrtc::PacedPacketInfo& pacingInfo) override;

  /* Pure virtual methods inherited from RTC::Timer. */
 public:
  void OnTimer(UvTimer* timer) override;

 private:
  // uv loop
  UvLoop* uv_loop_;

  // Passed by argument.
  Listener* listener{nullptr};
  // Allocated by this.
  webrtc::NetworkControllerFactoryInterface* controllerFactory{nullptr};
  webrtc::RtpTransportControllerSend* rtpTransportControllerSend{nullptr};
  UvTimer* processTimer{nullptr};

  // Others.
  uint32_t initialAvailableBitrate{0u};
  uint32_t maxOutgoingBitrate{0u};
  Bitrates bitrates;
  bool availableBitrateEventCalled{false};
  uint64_t lastAvailableBitrateEventAtMs{0u};
  std::deque<double> packetLossHistory;
  double packetLoss{0};
};
}  // namespace bifrost

#endif

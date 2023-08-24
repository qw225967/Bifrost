#ifndef MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP
#define MS_RTC_TRANSPORT_CONGESTION_CONTROL_CLIENT_HPP

#include <api/transport/goog_cc_factory.h>
#include <api/transport/network_types.h>
#include <call/rtp_transport_controller_send.h>
#include <modules/pacing/packet_router.h>

#include <deque>

#include "bifrost/bifrost_send_algorithm/bifrost_send_algorithm_interface.h"
#include "common.h"
#include "rtcp_tcc.h"
#include "rtp_packet.h"
#include "uv_timer.h"

namespace bifrost {
class TransportCongestionControlClient
    : public BifrostSendAlgorithmInterface,
      public webrtc::PacketRouter,
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
  class Observer {
   public:
    virtual ~Observer() = default;

   public:
    virtual void OnTransportCongestionControlClientBitrates(
        TransportCongestionControlClient* tcc_client,
        TransportCongestionControlClient::Bitrates& bitrates) = 0;
    virtual void OnTransportCongestionControlClientSendRtpPacket(
        TransportCongestionControlClient* tcc_client, RtpPacket* packet,
        const webrtc::PacedPacketInfo& pacing_info) = 0;
  };
 public:
  // BifrostSendAlgorithmInterface
  void OnRtpPacketSend(RtpPacketPtr rtp_packet, int64_t now) override;
  void UpdateRtt(float rtt) override {}

  bool OnReceiveRtcpFeedback(FeedbackRtpPacket* fb) override {
    if (fb->GetMessageType() == FeedbackRtp::MessageType::TCC) {
      auto* feedback = dynamic_cast<FeedbackRtpTransportPacket*>(fb);
      this->ReceiveRtcpTransportFeedback(feedback);
      return true;
    }
    return false;
  }
  void OnReceiveReceiverReport(webrtc::RTCPReportBlock report,
                               float rtt, int64_t nowMs) override {
    this->ReceiveRtcpReceiverReport(report, rtt, nowMs);
  }
  uint32_t get_pacing_rate() override { return this->get_available_bitrate(); }
  uint32_t get_congestion_windows() { return 0; }
  uint32_t get_bytes_in_flight() { return 0; }
  uint32_t get_pacing_transfer_time(uint32_t bytes) { return 0; }
  std::vector<double> get_trends() override {
    return this->get_trend();
  }

 public:
  TransportCongestionControlClient(
      TransportCongestionControlClient::Observer* observer,
      uint32_t initial_available_bitrate, UvLoop** uv_loop);
  ~TransportCongestionControlClient() override;

 public:
  const Bitrates& get_bit_rates() const { return this->bitrates_; }
  double get_packet_loss() const { return this->packet_loss_; }
  uint32_t get_available_bitrate() const {
    return this->bitrates_.availableBitrate;
  }
  std::vector<double> get_trend() {
    return rtp_transport_controller_send_->GetPrevTrend();
  }

 public:
  void InsertPacket(webrtc::RtpPacketSendInfo& packetInfo);
  webrtc::PacedPacketInfo GetPacingInfo();
  void PacketSent(webrtc::RtpPacketSendInfo& packetInfo, int64_t nowMs);
  void ReceiveEstimatedBitrate(uint32_t bitrate);
  void ReceiveRtcpTransportFeedback(const FeedbackRtpTransportPacket* feedback);
  void RescheduleNextAvailableBitrateEvent();
  void ReceiveRtcpReceiverReport(const webrtc::RTCPReportBlock& report,
                                 float rtt, int64_t nowMs);

 private:
  void MayEmitAvailableBitrateEvent(uint32_t previousAvailableBitrate);
  void UpdatePacketLoss(double packetLoss);
  void InitializeController();
  void DestroyController();

  // jmillan: missing.
  // void OnRemoteNetworkEstimate(NetworkStateEstimate estimate) override;

  /* Pure virtual methods inherited from webrtc::TargetTransferRateObserver. */
 public:
  void OnTargetTransferRate(
      webrtc::TargetTransferRate targetTransferRate) override;

  /* Pure virtual methods inherited from webrtc::PacketRouter. */
  void OnStartRateUpdate(webrtc::DataRate) override {}

 public:
  void SendPacket(RtpPacket* packet,
                  const webrtc::PacedPacketInfo& cluster_info) override;

  RtpPacket* GeneratePadding(size_t target_size_bytes) override;

  /* Pure virtual methods inherited from RTC::Timer. */
 public:
  void OnTimer(UvTimer* timer) override;

 private:
  // uv loop
  UvLoop* uv_loop_;

  // Passed by argument.
  Observer* observer_{nullptr};
  // Allocated by this.
  webrtc::NetworkControllerFactoryInterface* controller_factory_{nullptr};
  webrtc::RtpTransportControllerSend* rtp_transport_controller_send_{nullptr};
  UvTimer* process_timer_{nullptr};

  // Others.
  uint32_t initial_available_bitrate_{0u};
  uint32_t max_outgoing_bitrate_{0u};
  Bitrates bitrates_;
  bool available_bitrate_event_called_{false};
  uint64_t last_available_bitrate_event_at_ms_{0u};
  std::deque<double> packet_loss_history_;
  double packet_loss_{0};
};
}  // namespace bifrost

#endif

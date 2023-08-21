/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : publisher.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_PUBLISHER_H
#define WORKER_PUBLISHER_H

#include <map>


#include "bifrost/bifrost_send_algorithm/tcc_client.h"
#include "bifrost/experiment_manager/experiment_manager.h"
#include "bifrost_send_algorithm/bifrost_send_algorithm_manager.h"
#include "data_producer.h"
#include "nack.h"
#include "rtcp_rr.h"
#include "rtcp_sr.h"
#include "rtcp_tcc.h"
#include "setting.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<BifrostSendAlgorithmManager>
    BifrostSendAlgorithmManagerPtr;
typedef std::shared_ptr<Nack> NackPtr;
class Publisher : public UvTimer::Listener,
                  public TransportCongestionControlClient::Observer {
 public:
  class Observer {
   public:
    virtual void OnPublisherSendPacket(RtpPacketPtr packet,
                                       const struct sockaddr* remote_addr) = 0;
    virtual void OnPublisherSendRtcpPacket(
        CompoundPacketPtr packet, const struct sockaddr* remote_addr) = 0;
  };

 public:
  void OnReceiveRtcpFeedback(const FeedbackRtpPacket* fb) {
    bifrost_send_algorithm_manager_->OnReceiveRtcpFeedback(fb);
  }
  void OnReceiveNack(FeedbackRtpNackPacket* packet);
  void OnReceiveReceiverReport(ReceiverReport* report);

  SenderReport* GetRtcpSenderReport(uint64_t nowMs);

 public:
  Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
            Observer* observer, uint8_t number,
            ExperimentManagerPtr& experiment_manager,
            quic::CongestionControlType quic_congestion_type);
  ~Publisher() {
    delete send_report_timer_;
  }
  // UvTimer
  void OnTimer(UvTimer* timer) override;

  // TransportCongestionControlClient::Observer
  void OnTransportCongestionControlClientBitrates(
      TransportCongestionControlClient* tcc_client,
      TransportCongestionControlClient::Bitrates& bitrates) override {}
  void OnTransportCongestionControlClientSendRtpPacket(
      TransportCongestionControlClient* tcc_client, RtpPacket* packet,
      const webrtc::PacedPacketInfo& pacing_info) override {}

 private:
  uint32_t TimerSendPacket(int32_t available);
  void RemoveOldSendPacket();
  void GetRtpExtensions(RtpPacketPtr &packet);

 private:
  /* ------------ base ------------ */
  // observer
  Observer* observer_;
  // remote addr
  SockAddressPtr udp_remote_address_;
  // addr config
  Settings::Configuration remote_addr_config_;
  // uv
  UvLoop* uv_loop_;
  UvTimer* send_report_timer_;
  // ssrc
  uint32_t ssrc_;
  // number
  uint8_t number_;
  /* ------------ base ------------ */

  /* ------------ bifrost send algorithm manager ------------ */
  BifrostSendAlgorithmManagerPtr bifrost_send_algorithm_manager_;
  /* ------------ bifrost send algorithm manager ------------ */

  /* ------------ experiment ------------ */
  // experiment manager
  ExperimentManagerPtr experiment_manager_;
  // sr
  uint32_t send_packet_count_{0u};
  uint64_t send_packet_bytes_{0u};
  uint64_t max_packet_ms_{0u};
  uint64_t max_packet_ts_{0u};
  // send report
  float rtt_ = 0;
  // nack
  NackPtr nack_;

  /* ------------ experiment ------------ */
};
}  // namespace bifrost

#endif  // WORKER_PUBLISHER_H

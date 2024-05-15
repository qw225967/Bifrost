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

#include "bifrost_send_algorithm/bifrost_pacer.h"
#include "bifrost_send_algorithm/bifrost_send_algorithm_manager.h"
#include "nack.h"
#include "rtcp_rr.h"
#include "rtcp_sr.h"
#include "rtcp_tcc.h"
#include "setting.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;     // 发送的socket
typedef std::shared_ptr<BifrostSendAlgorithmManager>  // 发送算法管理
    BifrostSendAlgorithmManagerPtr;
typedef std::shared_ptr<Nack> NackPtr;                  // nack 管理
typedef std::shared_ptr<BifrostPacer> BifrostPacerPtr;  // 发送类

class Publisher : public UvTimer::Listener, public BifrostPacer::Observer {
 public:
  class Observer {
   public:
    virtual void OnPublisherSendPacket(RtpPacketPtr packet,
                                       const struct sockaddr* remote_addr) = 0;
    virtual void OnPublisherSendRtcpPacket(
        CompoundPacketPtr packet, const struct sockaddr* remote_addr) = 0;
  };

 public:
  // BifrostPacer::Observer
  void OnPublisherSendPacket(RtpPacketPtr packet) override {
    // 发送算法需要记录发送内容
    this->bifrost_send_algorithm_manager_->OnRtpPacketSend(
        packet, this->uv_loop_->get_time_ms_int64());

    if (packet->GetPayloadType() == 110)
      this->fec_nack_->OnSendRtpPacket(packet);
    else
      this->nack_->OnSendRtpPacket(packet);

    this->observer_->OnPublisherSendPacket(packet,
                                           this->udp_remote_address_.get());
  }
  void OnPublisherSendReTransPacket(RtpPacketPtr packet) override {
    // 发送算法需要记录发送内容
    this->bifrost_send_algorithm_manager_->OnRtpPacketSend(
        packet, this->uv_loop_->get_time_ms_int64());

    this->observer_->OnPublisherSendPacket(packet,
                                           this->udp_remote_address_.get());
  }

  void OnPublisherSendRtcpPacket(CompoundPacketPtr packet) override {
    this->observer_->OnPublisherSendRtcpPacket(packet,
                                               this->udp_remote_address_.get());
  }

 public:
  void OnReceiveRtcpFeedback(FeedbackRtpPacket* fb);
  void OnReceiveNack(FeedbackRtpNackPacket* packet);
  void OnReceiveReceiverReport(ReceiverReport* report);

  SenderReport* GetRtcpSenderReport(uint64_t nowMs) const;

 public:
  Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
            Observer* observer, uint8_t number,
            ExperimentManagerPtr& experiment_manager,
            quic::CongestionControlType quic_congestion_type);
  ~Publisher() override {
    delete send_report_timer_;
    delete update_pacing_info_timer_;
    pacer_.reset();
  }
  // UvTimer
  void OnTimer(UvTimer* timer) override;

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
  UvTimer* update_pacing_info_timer_;
  // ssrc
  uint32_t ssrc_;

  uint32_t fec_ssrc_;
  // number
  uint8_t number_;
  /* ------------ bifrost send algorithm manager ------------ */
  BifrostSendAlgorithmManagerPtr bifrost_send_algorithm_manager_;

  /* ------------ pacer ------------ */
  BifrostPacerPtr pacer_;

  /* ------------ experiment manger ------------ */
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

  // nack + fec
  NackPtr fec_nack_;
};
}  // namespace bifrost

#endif  // WORKER_PUBLISHER_H

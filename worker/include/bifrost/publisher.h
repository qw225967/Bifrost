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
typedef std::shared_ptr<DataProducer> DataProducerPtr;
typedef std::shared_ptr<TransportCongestionControlClient>
    TransportCongestionControlClientPtr;
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
  void ReceiveFeedbackTransport(const FeedbackRtpTransportPacket* feedback) {
    if (this->congestion_type_ != quic::kGoogCC)
      return;

    if (this->tcc_client_ != nullptr) {
      this->tcc_client_->ReceiveRtcpTransportFeedback(feedback);
      this->pacer_bits_ = this->tcc_client_->get_available_bitrate();
    }
  }
  void OnReceiveNack(FeedbackRtpNackPacket* packet);
  void OnReceiveReceiverReport(ReceiverReport* report);
  void OnSendPacketInNack(RtpPacketPtr& packet) {
    nack_->OnSendRtpPacket(packet);
  }

  SenderReport* GetRtcpSenderReport(uint64_t nowMs);

 public:
  Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
            Observer* observer, uint8_t number,
            ExperimentManagerPtr& experiment_manager,
            quic::CongestionControlType quic_congestion_type);
  ~Publisher() {
    pacer_vec_.clear();
    delete producer_timer_;
    delete data_dump_timer_;
    delete send_report_timer_;

    if (tcc_client_ != nullptr) tcc_client_.reset();

    data_producer_.reset();
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
  UvTimer* producer_timer_;
  UvTimer* data_dump_timer_;
  UvTimer* send_report_timer_;
  // ssrc
  uint32_t ssrc_;
  // number
  uint8_t number_;
  // experiment manager
  ExperimentManagerPtr experiment_manager_;
  /* ------------ base ------------ */

  /* ------------ experiment ------------ */
  // pacer vec
  std::vector<RtpPacketPtr> pacer_vec_;
  // sr
  uint32_t send_packet_count_{0u};
  uint64_t send_packet_bytes_{0u};
  uint64_t max_packet_ms_{0u};
  uint64_t max_packet_ts_{0u};
  // send packet producer
  DataProducerPtr data_producer_;
  // send report
  float rtt_ = 0;
  // congestion_type
  quic::CongestionControlType congestion_type_;
  // tcc
  uint16_t tcc_seq_ = 0;
  TransportCongestionControlClientPtr tcc_client_{nullptr};
  // pacer bytes
  uint32_t pacer_bits_;
  int32_t pre_remind_bytes_ = 0;
  uint32_t send_bits_prior_ = 0;
  // nack
  NackPtr nack_;

  /* ------------ experiment ------------ */
};
}  // namespace bifrost

#endif  // WORKER_PUBLISHER_H

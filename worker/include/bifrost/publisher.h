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

#include "data_producer.h"
#include "rtcp_tcc.h"
#include "setting.h"
#include "tcc_client.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<DataProducer> DataProducerPtr;
typedef std::shared_ptr<TransportCongestionControlClient>
    TransportCongestionControlClientPtr;
class Publisher : public UvTimer::Listener,
                  public TransportCongestionControlClient::Observer {
 public:
  class Observer {
   public:
    virtual void OnPublisherSendPacket(RtpPacketPtr packet,
                                       const struct sockaddr* remote_addr) = 0;
  };

 public:
  void ReceiveFeedbackTransport(const FeedbackRtpTransportPacket* feedback) {
    this->tcc_client_->ReceiveRtcpTransportFeedback(feedback);
    this->pacer_bits_ = this->tcc_client_->get_available_bitrate();
  }

 public:
  Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
            Observer* observer);
  ~Publisher() {
    delete producer_timer;
    tcc_client_.reset();
    data_producer_.reset();
  }
  // UvTimer
  void OnTimer(UvTimer* timer);

  // TransportCongestionControlClient::Observer
  void OnTransportCongestionControlClientBitrates(
      TransportCongestionControlClient* tcc_client,
      TransportCongestionControlClient::Bitrates& bitrates) override {}
  void OnTransportCongestionControlClientSendRtpPacket(
      TransportCongestionControlClient* tcc_client, RtpPacket* packet,
      const webrtc::PacedPacketInfo& pacing_info) override {}

 private:
  void GetRtpExtensions(RtpPacketPtr packet);
  void TccClientSendRtpPacket(const uint8_t* data, size_t len);

 private:
  // observer
  Observer* observer_;

  // uv
  UvLoop* uv_loop_;
  UvTimer* producer_timer;

  // tcc
  uint16_t tcc_seq_ = 0;
  TransportCongestionControlClientPtr tcc_client_{nullptr};

  // send packet producer
  DataProducerPtr data_producer_;

  // remote addr
  SockAddressPtr udp_remote_address_;

  // addr config
  Settings::Configuration remote_addr_config_;

  // pacer bytes
  uint32_t pacer_bits_;
  int32_t pre_remind_bytes_ = 0;
};
}  // namespace bifrost

#endif  // WORKER_PUBLISHER_H

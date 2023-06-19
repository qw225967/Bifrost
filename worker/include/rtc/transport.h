/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:09 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : transport.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_TRANSPORT_H
#define WORKER_TRANSPORT_H

#include "data_producer.h"
#include "tcc_client.h"
#include "tcc_server.h"
#include "udp_router.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<UvTimer> UvTimerPtr;
typedef std::shared_ptr<UvLoop> UvLoopPtr;
typedef std::shared_ptr<UdpRouter> UdpRouterPtr;
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<DataProducer> DataProducerPtr;
typedef std::shared_ptr<TransportCongestionControlClient>
    TransportCongestionControlClientPtr;
typedef std::shared_ptr<TransportCongestionControlServer>
    TransportCongestionControlServerPtr;

class Transport : public UvTimer::Listener,
                  public UdpRouter::UdpRouterObServer,
                  public TransportCongestionControlClient::Observer,
                  public TransportCongestionControlServer::Observer {
 public:
  Transport(Settings::Configuration& local_config,
            Settings::Configuration& remote_config);
  ~Transport();

 public:
  // UvTimer
  void OnTimer(UvTimer* timer) override;

  // UdpRouterObServer
  void OnUdpRouterRtpPacketReceived(
      bifrost::UdpRouter* socket, RtpPacketPtr rtp_packet,
      const struct sockaddr* remote_addr) override;
  void OnUdpRouterRtcpPacketReceived(
      bifrost::UdpRouter* socket, RtcpPacketPtr rtcp_packet,
      const struct sockaddr* remote_addr) override;

  // TransportCongestionControlClient::Observer
  void OnTransportCongestionControlClientBitrates(
      TransportCongestionControlClient* tcc_client,
      TransportCongestionControlClient::Bitrates& bitrates) override {}
  void OnTransportCongestionControlClientSendRtpPacket(
      TransportCongestionControlClient* tcc_client, RtpPacket* packet,
      const webrtc::PacedPacketInfo& pacing_info) override {}

  // TransportCongestionControlServer::Observer
  void OnTransportCongestionControlServerSendRtcpPacket(
      TransportCongestionControlServer* tccServer,
      RtcpPacket* packet) override {}

  void Run();

  void RunDataProducer();

  void SetRemoteTransport(uint32_t ssrc, UdpRouter::UdpRouterObServerPtr observer);

 private:
  void TccClientSendRtpPacket(RtpPacketPtr& packet);

 private:
  // tcc
  uint16_t tcc_seq_ = 0;
  TransportCongestionControlClientPtr tcc_client_{nullptr};
  TransportCongestionControlServerPtr tcc_server_{nullptr};

  // send packet producer
  DataProducerPtr data_producer_;

  // uv
  UvTimerPtr producer_timer;
  UvLoopPtr uv_loop_;
  UdpRouterPtr udp_router_;
  SockAddressPtr udp_remote_address_;

  // addr
  Settings::Configuration local_;
  Settings::Configuration remote_;
};
}  // namespace bifrost

#endif  // WORKER_TRANSPORT_H

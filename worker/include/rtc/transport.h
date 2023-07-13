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

#include "experiment_manager.h"
#include "player.h"
#include "publisher.h"
#include "rtcp_compound_packet.h"
#include "udp_router.h"
#include "unordered_map"
#include "uv_loop.h"

namespace bifrost {
typedef std::shared_ptr<Player> PlayerPtr;
typedef std::shared_ptr<Publisher> PublisherPtr;
typedef std::shared_ptr<UdpRouter> UdpRouterPtr;
class Transport : public UdpRouter::UdpRouterObServer,
                  public Publisher::Observer,
                  public Player::Observer {
 public:
  enum TransportModel {
    SinglePublish,
    SinglePlay,
    SinglePublishAndPlays,
  };

 public:
  Transport(TransportModel model, uint8_t number,
            ExperimentManagerPtr& experiment_manager);
  ~Transport();

 public:
  // Publisher
  void OnPublisherSendPacket(RtpPacketPtr packet,
                             const struct sockaddr* remote_addr) override {
    this->udp_router_->Send(packet->GetData(), packet->GetSize(), remote_addr,
                            nullptr);
  }

  void OnPublisherSendRtcpPacket(CompoundPacketPtr packet,
                                 const struct sockaddr* remote_addr) override {
    this->udp_router_->Send(packet->GetData(), packet->GetSize(), remote_addr,
                            nullptr);
  }

  // Player
  void OnPlayerSendPacket(RtcpPacketPtr packet,
                          const struct sockaddr* remote_addr) override {
    this->udp_router_->Send(packet->GetData(), packet->GetSize(), remote_addr,
                            nullptr);
  }

  // UdpRouterObServer
  void OnUdpRouterRtpPacketReceived(
      bifrost::UdpRouter* socket, RtpPacketPtr rtp_packet,
      const struct sockaddr* remote_addr) override;
  void OnUdpRouterRtcpPacketReceived(
      bifrost::UdpRouter* socket, RtcpPacketPtr rtcp_packet,
      const struct sockaddr* remote_addr) override;

  void Run();

 private:
  // uv
  UvLoop* uv_loop_;

  // router
  UdpRouterPtr udp_router_;

  // players
  std::unordered_map<uint32_t, PlayerPtr> players_;

  // publisher
  PublisherPtr publisher_;

  // TransportModel
  TransportModel model_;

  // number
  uint8_t number_;

  // experiment manager
  ExperimentManagerPtr experiment_manager_;
};
}  // namespace bifrost

#endif  // WORKER_TRANSPORT_H

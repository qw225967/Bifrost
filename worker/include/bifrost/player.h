/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : player.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_PLAYER_H
#define WORKER_PLAYER_H

#include "setting.h"
#include "tcc_server.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<TransportCongestionControlServer>
    TransportCongestionControlServerPtr;
class Player : public UvTimer::Listener,
               public TransportCongestionControlServer::Observer {
 public:
  class Observer {
   public:
    virtual void OnPlayerSendPacket(RtcpPacketPtr packet,
                                    const struct sockaddr* remote_addr) = 0;
  };

 public:
  Player(const struct sockaddr* remote_addr, UvLoop** uv_loop,
         Observer* observer);
  ~Player() {}

  // UvTimer
  void OnTimer(UvTimer* timer) {}

  // TransportCongestionControlServer::Observer
  void OnTransportCongestionControlServerSendRtcpPacket(
      TransportCongestionControlServer* tccServer,
      RtcpPacketPtr packet) override {
    packet->Serialize(Buffer);
    observer_->OnPlayerSendPacket(packet, udp_remote_address_.get());
  }

 public:
  void IncomingPacket(RtpPacketPtr packet) {
    this->tcc_server_->IncomingPacket(this->uv_loop_->get_time_ms_int64(),
                                      packet.get());
  }

 private:
  // observer
  Observer* observer_;

  // uv
  UvLoop* uv_loop_;

  // tcc
  TransportCongestionControlServerPtr tcc_server_{nullptr};

  // remote addr
  SockAddressPtr udp_remote_address_;
};
}  // namespace bifrost

#endif  // WORKER_PLAYER_H

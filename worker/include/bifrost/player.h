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

#include "nack.h"
#include "setting.h"
#include "tcc_server.h"
#include "rtcp_sr.h"
#include "rtcp_rr.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<Nack> NackPtr;
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
         Observer* observer, uint32_t ssrc);
  ~Player() { this->nack_.reset(); }

  // UvTimer
  void OnTimer(UvTimer* timer) {}

  // TransportCongestionControlServer::Observer
  void OnTransportCongestionControlServerSendRtcpPacket(
      TransportCongestionControlServer* tccServer,
      RtcpPacketPtr packet) override {
    packet->Serialize(Buffer);
    observer_->OnPlayerSendPacket(packet, udp_remote_address_.get());
  }

  void OnReceiveRtpPacket(RtpPacketPtr packet);

  void OnSendRtcpNack(RtcpPacketPtr packet) {
    observer_->OnPlayerSendPacket(packet, udp_remote_address_.get());
  }

  void OnReceiveSenderReport(SenderReport* report);

  ReceiverReport* GetRtcpReceiverReport();

 public:
 private:
  /* ------------ base ------------ */
  // observer
  Observer* observer_;
  // remote addr
  SockAddressPtr udp_remote_address_;
  // uv
  UvLoop* uv_loop_;
  // ssrc
  uint32_t ssrc_;
  /* ------------ base ------------ */

  /* ------------ experiment ------------ */
  // sr
  uint32_t last_sr_timestamp;
  uint32_t last_sender_repor_ts;
  uint64_t last_sr_received;
  uint64_t last_sender_reportNtpMs;
  // nack
  NackPtr nack_;
  // tcc
  TransportCongestionControlServerPtr tcc_server_{nullptr};
  /* ------------ experiment ------------ */
};
}  // namespace bifrost

#endif  // WORKER_PLAYER_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:32 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : client_server.h
 * @description : 这是一个可多次申请的udp路由类，当处于多接发模式时使用
 *                该类主要用于需要独立使用socket的方式
 *******************************************************/

#ifndef WORKER_CLIENT_SERVER_H
#define WORKER_CLIENT_SERVER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "port_manager.h"
#include "udp_socket.h"

namespace bifrost {

class UdpRouter : public UdpSocket {
 public:
  class UdpRouterObServer {
   public:
    virtual void OnUdpRouterPacketReceived(
        bifrost::UdpRouter* socket, const uint8_t* data, size_t len,
        const struct sockaddr* remoteAddr) = 0;
  };
  typedef std::shared_ptr<UdpRouterObServer> UdpRouterObServerPtr;

  /* Instance methods. */
  UdpRouter(uv_loop_t* loop) : UdpSocket(PortManager::BindUdp(loop)) {}
  UdpRouter(Settings::Configuration config, uv_loop_t* loop)
      : UdpSocket(PortManager::BindUdp(std::move(config), loop)) {}

 public:
  UdpRouter(UdpRouter& other) = delete;
  void operator=(const UdpRouter&) = delete;

 public:
  ~UdpRouter() override;

  /* Pure virtual methods inherited from ::UdpRouter. */
 public:
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;
  //  RTC::RTCP::Packet* RtcpDataReceived(const uint8_t* data, size_t len);
  //  RTC::RtpPacket* RtpDataReceived(const uint8_t* data, size_t len);
 private:
  UdpRouterObServerPtr observer;
};
}  // namespace bifrost

#endif  // WORKER_CLIENT_SERVER_H

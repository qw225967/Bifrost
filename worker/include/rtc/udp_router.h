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

#include "common.h"
#include "port_manager.h"
#include "udp_socket.h"

namespace bifrost {
class UdpRouter : public UdpSocket {
 public:
  class UdpRouterObServer {
   public:
    virtual void OnUdpRouterRtpPacketReceived(
        bifrost::UdpRouter* socket, RtpPacketPtr rtp_packet,
        const struct sockaddr* remote_addr) = 0;
    virtual void OnUdpRouterRtcpPacketReceived(
        bifrost::UdpRouter* socket, RtcpPacketPtr rtcp_packet,
        const struct sockaddr* remote_addr) = 0;
  };
  typedef std::shared_ptr<UdpRouterObServer> UdpRouterObServerPtr;

 public:
  /* Instance methods. */
  UdpRouter(uv_loop_t* loop, UdpRouterObServer* observer)
      : observer_(observer), UdpSocket(PortManager::BindUdp(loop)) {}
  UdpRouter(Settings::Configuration config, uv_loop_t* loop,
            UdpRouterObServer* observer)
      : observer_(observer),
        UdpSocket(PortManager::BindUdp(std::move(config), loop)) {}
  UdpRouter(UdpRouter& other) = delete;
  void operator=(const UdpRouter&) = delete;
  ~UdpRouter() override;

  /* Pure virtual methods inherited from ::UdpRouter. */
 public:
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;

 private:
  UdpRouterObServer* observer_;
};
}  // namespace bifrost

#endif  // WORKER_CLIENT_SERVER_H

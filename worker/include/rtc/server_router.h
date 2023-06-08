/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:32 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : server_router.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_SERVER_ROUTER_H
#define WORKER_SERVER_ROUTER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "port_manager.h"
#include "udp_socket.h"

namespace bifrost {
class ServerRouter : public UdpSocket {
 public:
  class ServerRouterObServer {
   public:
    virtual void OnServerRouterPacketReceived(
        bifrost::ServerRouter* socket, const uint8_t* data, size_t len,
        const struct sockaddr* remoteAddr) = 0;
  };

 protected:
  static ServerRouter* udp_router_;
  /* Instance methods. */
  ServerRouter() : UdpSocket(PortManager::BindUdp()) {}

 public:
  ServerRouter(ServerRouter& other) = delete;
  void operator=(const ServerRouter&) = delete;
  /*save ssrc pointer transport*/
  static ServerRouter* GetServerRouter();

 public:
  ~ServerRouter() override;

  /* Pure virtual methods inherited from ::ServerRouter. */
 public:
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;
  //  RTC::RTCP::Packet* RtcpDataReceived(const uint8_t* data, size_t len);
  //  RTC::RtpPacket* RtpDataReceived(const uint8_t* data, size_t len);

 public:
  // Passed by argument.
  std::unordered_map<uint32_t, ServerRouterObServer*> observer_;

 private:
  std::unordered_map<std::string, ServerRouterObServer*> username_map_;
  std::unordered_map<std::string, ServerRouterObServer*> peer_map_;
  std::unordered_map<std::string, std::vector<std::string>*> user_peer_map_;
};
}  // namespace bifrost

#endif  // WORKER_SERVER_ROUTER_H

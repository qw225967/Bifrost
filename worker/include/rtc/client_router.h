/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:32 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : client_server.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_CLIENT_SERVER_H
#define WORKER_CLIENT_SERVER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "port_manager.h"
#include "udp_socket.h"

namespace bifrost {
class ClientRouter : public UdpSocket {
 public:
  class ClientRouterObServer {
   public:
    virtual void OnClientRouterPacketReceived(
        bifrost::ClientRouter* socket, const uint8_t* data, size_t len,
        const struct sockaddr* remoteAddr) = 0;
  };

 protected:
  /* Instance methods. */
  ClientRouter() : UdpSocket(PortManager::BindUdp()) {}

 public:
  ClientRouter(ClientRouter& other) = delete;
  void operator=(const ClientRouter&) = delete;

 public:
  ~ClientRouter() override;

  /* Pure virtual methods inherited from ::ClientRouter. */
 public:
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;
  //  RTC::RTCP::Packet* RtcpDataReceived(const uint8_t* data, size_t len);
  //  RTC::RtpPacket* RtpDataReceived(const uint8_t* data, size_t len);

 public:
  // Passed by argument.
  std::unordered_map<uint32_t, ClientRouterObServer*> observer_;

 private:
  std::unordered_map<std::string, ClientRouterObServer*> username_map_;
  std::unordered_map<std::string, ClientRouterObServer*> peer_map_;
  std::unordered_map<std::string, std::vector<std::string>*> user_peer_map_;
};
}  // namespace bifrost

#endif  // WORKER_CLIENT_SERVER_H

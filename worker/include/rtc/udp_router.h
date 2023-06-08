/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 1:57 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : udp_router.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_UDP_ROUTER_H
#define WORKER_UDP_ROUTER_H

#include <unordered_map>

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

 protected:
  static UdpRouter* udp_router_;
  /* Instance methods. */
  UdpRouter() : UdpSocket(PortManager::BindUdp()) {}

 public:
  UdpRouter(UdpRouter& other) = delete;
  void operator=(const UdpRouter&) = delete;
  /*save ssrc pointer transport*/
  static UdpRouter* GetUdpRouter();

 public:
  ~UdpRouter() override;

  /* Pure virtual methods inherited from ::UdpRouter. */
 public:
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;
  //  RTC::RTCP::Packet* RtcpDataReceived(const uint8_t* data, size_t len);
  //  RTC::RtpPacket* RtpDataReceived(const uint8_t* data, size_t len);

 public:
  // Passed by argument.
  std::unordered_map<uint32_t, UdpRouterObServer*> observer_;

 private:
  std::unordered_map<std::string, UdpRouterObServer*> username_map_;
  std::unordered_map<std::string, UdpRouterObServer*> peer_map_;
  std::unordered_map<std::string, std::vector<std::string>*> user_peer_map_;
};
}  // namespace bifrost

#endif  // WORKER_UDP_ROUTER_H

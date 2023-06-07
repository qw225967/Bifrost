/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 1:57 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : udp_router.cpp
 * @description : TODO
 *******************************************************/

#include "udp_router.h"

#include "iostream"
#include "utils.h"

namespace bifrost {
UdpRouter* UdpRouter::udp_socket_ = nullptr;

UdpRouter::~UdpRouter() {
  delete udp_socket_;
  PortManager::UnbindUdp(this->localIp, this->localPort);

  peer_map_.clear();
  username_map_.clear();

  for (auto& kv : this->user_peer_map_) {
    auto* peers = kv.second;
    delete peers;
  }

  this->user_peer_map_.clear();
}

UdpRouter* UdpRouter::GetUdpRouter() {
  if (udp_socket_ == nullptr) {
    std::cout << "[setting] GetUdpRouter" << std::endl;
    udp_socket_ = new UdpRouter();
  }
  return udp_socket_;
}

void UdpRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                          const struct sockaddr* addr) {}
}  // namespace bifrost
/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:33 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : client_router.cpp
 * @description : TODO
 *******************************************************/

#include "client_router.h"

#include <iostream>

namespace bifrost {
ClientRouter::~ClientRouter() {
  PortManager::UnbindUdp(this->local_ip_, this->local_port_);

  peer_map_.clear();
  username_map_.clear();

  for (auto& kv : this->user_peer_map_) {
    auto* peers = kv.second;
    delete peers;
  }

  this->user_peer_map_.clear();
}

void ClientRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                             const struct sockaddr* addr) {
  std::cout << "UserOnUdpDatagramReceived data:" << data << std::endl;
}
}  // namespace bifrost
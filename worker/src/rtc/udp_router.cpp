/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:33 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : client_router.cpp
 * @description : TODO
 *******************************************************/

#include "udp_router.h"

#include <iostream>

namespace bifrost {
UdpRouter::~UdpRouter() {
  PortManager::UnbindUdp(this->local_ip_, this->local_port_);
}

void UdpRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                          const struct sockaddr* addr) {
  std::cout << "UserOnUdpDatagramReceived data:" << data << std::endl;
}
}  // namespace bifrost
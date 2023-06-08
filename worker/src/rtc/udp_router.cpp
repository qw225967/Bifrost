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
UdpRouter* UdpRouter::udp_router_ = nullptr;

UdpRouter::~UdpRouter() {
  delete udp_router_;
  PortManager::UnbindUdp(this->localIp, this->localPort);

  peer_map_.clear();
  username_map_.clear();

  for (auto& kv : this->user_peer_map_) {
    auto* peers = kv.second;
    delete peers;
  }

  this->user_peer_map_.clear();
}

// mediasoup 的垃圾实现，做单例没做好，static直接赋值会崩在主函数之前
// 只能在这里获取
UdpRouter* UdpRouter::GetUdpRouter() {
  if (udp_router_ == nullptr) {
    std::cout << "[udp router] GetUdpSocket" << std::endl;
    udp_router_ = new UdpRouter();
  }
  return udp_router_;
}

void UdpRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                          const struct sockaddr* addr) {}
}  // namespace bifrost
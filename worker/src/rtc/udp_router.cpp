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
#include <sstream>
#include <string>

#include "rtcp_packet.h"
#include "rtcp_tcc.h"
#include "rtp_packet.h"
#include "utils.h"

namespace bifrost {
UdpRouter::~UdpRouter() {
  PortManager::UnbindUdp(this->local_ip_, this->local_port_);
}

void UdpRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                          const struct sockaddr* remote_addr) {
  if (RtcpPacket::IsRtcp(data, len)) {
    auto rtcp_packet = RtcpPacket::Parse(data, len);
    if (rtcp_packet == nullptr) return;
    this->observer_->OnUdpRouterRtcpPacketReceived(this, rtcp_packet,
                                                   remote_addr);
  } else if (RtpPacket::IsRtp(data, len)) {
    auto rtp_packet = std::make_shared<RtpPacket>(data, len);
    if (rtp_packet == nullptr) return;
    this->observer_->OnUdpRouterRtpPacketReceived(this, rtp_packet,
                                                  remote_addr);
  }
}
}  // namespace bifrost
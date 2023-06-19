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

void UdpRouter::SetRemoteTransport(uint32_t ssrc,
                                   UdpRouterObServerPtr observer) {
  this->observers[ssrc] = std::move(observer);
}

void UdpRouter::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                          const struct sockaddr* remote_addr) {
  if (RtpPacket::IsRtp(data, len)) {
    auto rtp_packet = RtpPacket::Parse(data, len);
    auto iter = observers.find(rtp_packet->GetSsrc());
    if (iter != observers.end()) {
      iter->second->OnUdpRouterRtpPacketReceived(this, rtp_packet, remote_addr);
    }
  } else if (RtcpPacket::IsRtcp(data, len)) {
    auto rtcp_packet = RtcpPacket::Parse(data, len);

    auto type = rtcp_packet->GetType();
    switch (type) {
      case Type::RTPFB: {
        break;
      }
    }
  }
}
}  // namespace bifrost
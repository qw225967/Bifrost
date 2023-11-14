/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/13 4:34 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : ns3_proxy.cpp
 * @description : TODO
 *******************************************************/

#include "ns3_proxy.h"

#include "rtc_header.h"

namespace ns3proxy {
Ns3Proxy::Ns3Proxy() {}
Ns3Proxy::~Ns3Proxy() {}

bool Ns3Proxy::IsRtcp(const uint8_t* data, size_t len) {
  auto header =
      const_cast<RtcpHeader*>(reinterpret_cast<const RtcpHeader*>(data));

  // clang-format off
  return (
      (len >= sizeof(RtcpHeader)) &&
      // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
      (data[0] > 127 && data[0] < 192) &&
      // RTP Version must be 2.
      (header->version == 2) &&
      // RTCP packet types defined by IANA:
      // http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
      // RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
      (header->packetType >= 192 && header->packetType <= 223)
      );
  // clang-format on
}

bool Ns3Proxy::IsRtp(const uint8_t* data, size_t len) {
  // NOTE: RtcpPacket::IsRtcp() must always be called before this method.

  auto header =
      const_cast<RtpHeader*>(reinterpret_cast<const RtpHeader*>(data));

  // clang-format off
  return (
      (len >= RtpHeaderSize) &&
      // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
      (data[0] > 127 && data[0] < 192) &&
      // RTP Version must be 2.
      (header->version == 2)
      );
  // clang-format on
}

uint32_t Ns3Proxy::GetSsrcByData(const uint8_t* data, size_t len) {
  auto header =
      const_cast<RtcpHeader*>(reinterpret_cast<const RtcpHeader*>(data));
  if (this->IsRtcp(data, len)) {
    switch (header->packetType) {
      // sr
      case 200: {
        auto sr_header = const_cast<SRHeader*>(
            reinterpret_cast<const SRHeader*>(data + sizeof(RtcpHeader)));
        return sr_header->ssrc;
      }
      // rr
      case 201: {
        auto rr_header = const_cast<RRHeader*>(
            reinterpret_cast<const RRHeader*>(data + sizeof(RtcpHeader)));
        return rr_header->ssrc;
      }
      case 205: {
        auto fb_header = const_cast<FBHeader*>(
            reinterpret_cast<const FBHeader*>(data + sizeof(RtcpHeader)));
        return fb_header->media_ssrc;
      }
    }
  } else if (this->IsRtp(data, len)) {
    auto header =
        const_cast<RtpHeader*>(reinterpret_cast<const RtpHeader*>(data));

    return header->ssrc;
  }

  return 0;
}

void ProxyIn::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                        const struct sockaddr* addr) {
  auto ssrc = this->GetSsrcByData(data, len);
  this->observer_->ProxyInReceivePacket(ssrc, data, len, addr);
}

void ProxyOut::UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                         const struct sockaddr* addr) {
  auto ssrc = this->GetSsrcByData(data, len);
  this->observer_->ProxyOutReceivePacket(ssrc, data, len, addr);
}

ProxyManager::ProxyManager() {
  this->loop_ = new uv_loop_t;

  int err = uv_loop_init(loop);
  if (err != 0) std::cout << "[proxy] initialization failed" << std::endl;

  std::string ip("0.0.0.0");
  this->proxy_in_ = std::make_shared<ProxyIn>(ip, 9099, this, this->loop_in_);
  this->proxy_out_ = std::make_shared<ProxyOut>(ip, 9098, this, this->loop_out_);

  // 设置代理ip、端口
  struct sockaddr_storage remote_addr;
  int family = UvRun::get_family("10.0.0.100");
  switch (family) {
    case AF_INET: {
      err = uv_ip4_addr("10.0.0.100", 0,
                        reinterpret_cast<struct sockaddr_in*>(&remote_addr));
      std::cout << "[proxy] remote uv_ip4_addr" << std::endl;
      if (err != 0)
        std::cout << "[proxy] remote uv_ip4_addr() failed: " << uv_strerror(err)
                  << std::endl;

      (reinterpret_cast<struct sockaddr_in*>(&remote_addr))->sin_port =
          htons(8889);
      break;
    }
  }
  this->proxy_addr_ = reinterpret_cast<struct sockaddr*>(&remote_addr);
}

ProxyManager::~ProxyManager() {
  this->proxy_in_.reset();
  this->proxy_out_.reset();
}

void ProxyManager::ProxyInReceivePacket(uint32_t ssrc, const uint8_t* data,
                                        size_t len,
                                        const struct sockaddr* addr) {
  if (ssrc == 0) return;

  auto ite = ssrc_remote_map_.find(ssrc);
  if (ite == ssrc_remote_map_.end()) {
    std::string ip;
    uint16_t port;
    int family;
    UvRun::get_address_info(addr, family, ip, port);
    std::cout << "recv out ip:" << ip << ", port:" << port << std::endl;
    // 存储对应目标端口
    this->ssrc_remote_map_[ssrc] = addr;
  }

  this->proxy_out_->Send(data, len, this->proxy_addr_, nullptr);
}

void ProxyManager::ProxyOutReceivePacket(uint32_t ssrc, const uint8_t* data,
                                        size_t len,
                                        const struct sockaddr* addr) {
  if (ssrc == 0) return;

  auto ite = ssrc_remote_map_.find(ssrc);
  if (ite != ssrc_remote_map_.end()) {
    std::string ip;
    uint16_t port;
    int family;
    UvRun::get_address_info(ite->second, family, ip, port);
    std::cout << "send out ip:" << ip << ", port:" << port << std::endl;
    this->proxy_in_->Send(data, len, ite->second, nullptr);
  } else {
    std::cout << "ssrc:" << ssrc << std::endl;
  }

}

}  // namespace ns3proxy

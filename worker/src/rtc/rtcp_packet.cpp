/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:11 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_packet.h"

#include <iostream>

#include "rtcp_feedback.h"
#include "rtcp_sr.h"
#include "rtcp_rr.h"

namespace bifrost {
/* Namespace variables. */

uint8_t Buffer[BufferSize];

/* Class variables. */

// clang-format off
std::map<Type, std::string> RtcpPacket::type2String =
    {
    { Type::SR,    "SR"    },
    { Type::RR,    "RR"    },
    { Type::SDES,  "SDES"  },
    { Type::BYE,   "BYE"   },
    { Type::APP,   "APP"   },
    { Type::RTPFB, "RTPFB" },
    { Type::PSFB,  "PSFB"  },
    { Type::XR,    "XR"    },
    { Type::NAT,   "NAT"   }
    };
// clang-format on

/* Class methods. */

std::shared_ptr<RtcpPacket> RtcpPacket::Parse(const uint8_t* data, size_t len) {
  // First, Currently parsing and Last RTCP packets in the compound packet.
  std::shared_ptr<RtcpPacket> current{nullptr};

  while (len > 0u) {
    if (!RtcpPacket::IsRtcp(data, len)) {
      std::cout << "[rtcp packet] data is not a RTCP packet" << std::endl;
      std::cout << "no rtcp:" << Byte::bytes_to_hex(data, len) << " no rtcp" << std::endl;
      return nullptr;
    }

    auto* header =
        const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
    size_t packetLen = static_cast<size_t>(ntohs(header->length) + 1) * 4;

    if (len < packetLen) {
      std::cout << "[rtcp packet] packet length exceeds remaining data [len:"
                << len << ", "
                << "packet len:" << packetLen << "]";

      return nullptr;
    }

    switch (Type(header->packetType)) {
      case Type::SR: {
        current = SenderReportPacket::Parse(data, packetLen);
        break;
      }

      case Type::RR: {
        current = ReceiverReportPacket::Parse(data, packetLen);
        break;
      }

      case Type::SDES: {
        break;
      }

      case Type::BYE: {
        break;
      }

      case Type::NAT: {
        break;
      }

      case Type::APP: {
        break;
      }

      case Type::RTPFB: {
        current = FeedbackRtpPacket::Parse(data, packetLen);
        break;
      }

      case Type::PSFB: {
        break;
      }

      case Type::XR: {
        break;
      }

      default: {
        std::cout << "[rtcp packet] unknown RTCP packet type [packetType:"
                  << header->packetType << "]" << std::endl;
      }
    }

    if (!current) {
      std::string packetType = Type2String(Type(header->packetType));
      if (Type(header->packetType) == Type::PSFB) {
      } else if (Type(header->packetType) == Type::RTPFB) {
        packetType += " " + FeedbackRtpPacket::MessageType2String(
                                FeedbackRtp::MessageType(header->count));
      }

      std::cout << "[rtcp packet] error parsing " << packetType.c_str()
                << " RtcpPacket" << std::endl;

      return nullptr;
    }

    data += packetLen;
    len -= packetLen;
  }

  return current;
}

const std::string& RtcpPacket::Type2String(Type type) {
  static const std::string Unknown("UNKNOWN");

  auto it = RtcpPacket::type2String.find(type);

  if (it == RtcpPacket::type2String.end()) return Unknown;

  return it->second;
}

/* Instance methods. */

size_t RtcpPacket::Serialize(uint8_t* buffer) {
  this->header = reinterpret_cast<CommonHeader*>(buffer);

  size_t length = (GetSize() / 4) - 1;

  // Fill the common header.
  this->header->version = 2;
  this->header->padding = 0;
  this->header->count = static_cast<uint8_t>(GetCount());
  this->header->packetType = static_cast<uint8_t>(GetType());
  this->header->length = uint16_t{htons(length)};

  return sizeof(CommonHeader);
}
}  // namespace bifrost
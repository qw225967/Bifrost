/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:23 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_feedback.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_feedback.h"

#include <cstring>

#include "rtcp_nack.h"
#include "rtcp_tcc.h"
#include "rtcp_quic_feedback.h"

namespace bifrost {
/* Class methods. */

template <typename T>
const std::string& FeedbackPacket<T>::MessageType2String(
    typename T::MessageType type) {
  static const std::string Unknown("UNKNOWN");

  auto it = FeedbackPacket<T>::type2String.find(type);

  if (it == FeedbackPacket<T>::type2String.end()) return Unknown;

  return it->second;
}

/* Instance methods. */

template <typename T>
FeedbackPacket<T>::FeedbackPacket(CommonHeader* commonHeader)
    : RtcpPacket(commonHeader),
      messageType(typename T::MessageType(commonHeader->count)) {
  this->header = reinterpret_cast<Header*>(
      reinterpret_cast<uint8_t*>(commonHeader) + sizeof(CommonHeader));
}

template <typename T>
FeedbackPacket<T>::FeedbackPacket(typename T::MessageType messageType,
                                  uint32_t senderSsrc, uint32_t mediaSsrc)
    : RtcpPacket(rtcpType), messageType(messageType) {
  this->raw = new uint8_t[sizeof(Header)];
  this->header = reinterpret_cast<Header*>(this->raw);
  this->header->senderSsrc = uint32_t{htonl(senderSsrc)};
  this->header->mediaSsrc = uint32_t{htonl(mediaSsrc)};
}

template <typename T>
FeedbackPacket<T>::~FeedbackPacket<T>() {
  delete[] this->raw;
}

/* Instance methods. */

template <typename T>
size_t FeedbackPacket<T>::Serialize(uint8_t* buffer) {
  size_t offset = RtcpPacket::Serialize(buffer);

  // Copy the header.
  std::memcpy(buffer + offset, this->header, sizeof(Header));

  return offset + sizeof(Header);
}

template <typename T>
void FeedbackPacket<T>::Dump() const {}

/* Specialization for Ps class. */

template <>
Type FeedbackPacket<FeedbackPs>::rtcpType = Type::PSFB;

// clang-format off
    template<>
    std::map<FeedbackPs::MessageType, std::string> FeedbackPacket<FeedbackPs>::type2String =
        {
        { FeedbackPs::MessageType::PLI,   "PLI"   },
        { FeedbackPs::MessageType::SLI,   "SLI"   },
        { FeedbackPs::MessageType::RPSI,  "RPSI"  },
        { FeedbackPs::MessageType::FIR,   "FIR"   },
        { FeedbackPs::MessageType::TSTR,  "TSTR"  },
        { FeedbackPs::MessageType::TSTN,  "TSTN"  },
        { FeedbackPs::MessageType::VBCM,  "VBCM"  },
        { FeedbackPs::MessageType::PSLEI, "PSLEI" },
        { FeedbackPs::MessageType::ROI,   "ROI"   },
        { FeedbackPs::MessageType::AFB,   "AFB"   },
        { FeedbackPs::MessageType::EXT,   "EXT"   }
        };
// clang-format on

template <>
std::shared_ptr<FeedbackPacket<FeedbackPs>> FeedbackPacket<FeedbackPs>::Parse(
    const uint8_t* data, size_t len) {
  if (len < sizeof(CommonHeader) + sizeof(FeedbackPacket::Header)) {
    std::cout
        << "[feedback packet] not enough space for Feedback packet, discarded"
        << std::endl;

    return nullptr;
  }

  auto* commonHeader =
      const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));
  FeedbackPsPacket* packet{nullptr};

  switch (FeedbackPs::MessageType(commonHeader->count)) {
    case FeedbackPs::MessageType::PLI:
      break;

    case FeedbackPs::MessageType::SLI:
      break;

    case FeedbackPs::MessageType::RPSI:
      break;

    case FeedbackPs::MessageType::FIR:
      break;

    case FeedbackPs::MessageType::TSTR:
      break;

    case FeedbackPs::MessageType::TSTN:
      break;

    case FeedbackPs::MessageType::VBCM:
      break;

    case FeedbackPs::MessageType::PSLEI:
      break;

    case FeedbackPs::MessageType::ROI:
      break;

    case FeedbackPs::MessageType::AFB:
      break;

    case FeedbackPs::MessageType::EXT:
      break;

    default:
      std::cout << "[feedback packet] unknown RTCP PS Feedback message type "
                   "[packetType:"
                << commonHeader->count << "]" << std::endl;
  }

  return nullptr;
}

/* Specialization for Rtcp class. */

template <>
Type FeedbackPacket<FeedbackRtp>::rtcpType = Type::RTPFB;

// clang-format off
    template<>
    std::map<FeedbackRtp::MessageType, std::string> FeedbackPacket<FeedbackRtp>::type2String =
        {
        { FeedbackRtp::MessageType::NACK,   "NACK"   },
        { FeedbackRtp::MessageType::TMMBR,  "TMMBR"  },
        { FeedbackRtp::MessageType::TMMBN,  "TMMBN"  },
        { FeedbackRtp::MessageType::SR_REQ, "SR_REQ" },
        { FeedbackRtp::MessageType::RAMS,   "RAMS"   },
        { FeedbackRtp::MessageType::TLLEI,  "TLLEI"  },
        { FeedbackRtp::MessageType::ECN,    "ECN"    },
        { FeedbackRtp::MessageType::PS,     "PS"     },
        { FeedbackRtp::MessageType::EXT,    "EXT"    },
        { FeedbackRtp::MessageType::TCC,    "TCC"    },
        { FeedbackRtp::MessageType::QUICFB, "QUICFB" }
        };
// clang-format on

/* Class methods. */

template <>
std::shared_ptr<FeedbackPacket<FeedbackRtp>> FeedbackPacket<FeedbackRtp>::Parse(
    const uint8_t* data, size_t len) {
  if (len < sizeof(CommonHeader) + sizeof(FeedbackPacket::Header)) {
    std::cout
        << "[feedback packet] not enough space for Feedback packet, discarded"
        << std::endl;

    return nullptr;
  }

  auto* commonHeader =
      reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(data));
  std::shared_ptr<FeedbackRtpPacket> packet{nullptr};

  switch (FeedbackRtp::MessageType(commonHeader->count)) {
    case FeedbackRtp::MessageType::NACK:
      packet = FeedbackRtpNackPacket::Parse(data, len);
      break;

    case FeedbackRtp::MessageType::TMMBR:
      break;

    case FeedbackRtp::MessageType::TMMBN:
      break;

    case FeedbackRtp::MessageType::SR_REQ:
      break;

    case FeedbackRtp::MessageType::RAMS:
      break;

    case FeedbackRtp::MessageType::TLLEI:
      break;

    case FeedbackRtp::MessageType::ECN:
      break;

    case FeedbackRtp::MessageType::PS:
      break;

    case FeedbackRtp::MessageType::EXT:
      break;

    case FeedbackRtp::MessageType::TCC:
      packet = FeedbackRtpTransportPacket::Parse(data, len);
      break;

    case FeedbackRtp::MessageType::QUICFB:
      packet = QuicAckFeedbackPacket::Parse(data, len);
      break;

    default:
      std::cout << "[feedback packet] unknown RTCP RTP Feedback message type "
                   "[packetType:"
                << commonHeader->count << "]" << std::endl;
  }

  return packet;
}

// Explicit instantiation to have all FeedbackPacket definitions in this file.
template class FeedbackPacket<FeedbackPs>;
template class FeedbackPacket<FeedbackRtp>;
}  // namespace bifrost
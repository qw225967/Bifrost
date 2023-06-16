/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:11 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_H
#define WORKER_RTCP_H

#include <map>
#include <string>

#include "common.h"

namespace bifrost {
// Internal buffer for RTCP serialization.
constexpr size_t BufferSize{65536};
extern uint8_t Buffer[BufferSize];

// Maximum interval for regular RTCP mode.
constexpr uint16_t MaxAudioIntervalMs{5000};
constexpr uint16_t MaxVideoIntervalMs{1000};

enum class Type : uint8_t {
  SR = 200,
  RR = 201,
  SDES = 202,
  BYE = 203,
  APP = 204,
  RTPFB = 205,
  PSFB = 206,
  XR = 207,
  NAT = 211
};

class RtcpPacket {
 public:
  /* Struct for RTCP common header. */
  struct CommonHeader {
#if defined(MS_LITTLE_ENDIAN)
    uint8_t count : 5;
    uint8_t padding : 1;
    uint8_t version : 2;
#elif defined(MS_BIG_ENDIAN)
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t count : 5;
#endif
    uint8_t packetType : 8;
    uint16_t length : 16;
  };

 public:
  static bool IsRtcp(const uint8_t* data, size_t len) {
    auto header =
        const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

    // clang-format off
        return (
            (len >= sizeof(CommonHeader)) &&
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
  static RtcpPacket* Parse(const uint8_t* data, size_t len);
  static const std::string& Type2String(Type type);

 private:
  static std::map<Type, std::string> type2String;

 public:
  explicit RtcpPacket(Type type) : type(type) {}
  explicit RtcpPacket(CommonHeader* commonHeader) {
    this->type = Type(commonHeader->packetType);
    this->header = commonHeader;
  }
  virtual ~RtcpPacket() = default;

  void SetNext(RtcpPacket* packet) { this->next = packet; }
  RtcpPacket* GetNext() const { return this->next; }
  const uint8_t* GetData() const {
    return reinterpret_cast<uint8_t*>(this->header);
  }

 public:
  virtual void Dump() const = 0;
  virtual size_t Serialize(uint8_t* buffer) = 0;
  virtual Type GetType() const { return this->type; }
  virtual size_t GetCount() const { return 0u; }
  virtual size_t GetSize() const = 0;

 private:
  Type type;
  RtcpPacket* next{nullptr};
  CommonHeader* header{nullptr};
};
}  // namespace bifrost

#endif  // WORKER_RTCP_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:23 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_feedback.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_FEEDBACK_H
#define WORKER_RTCP_FEEDBACK_H

#include "rtcp_packet.h"

namespace bifrost {
template <typename T>
class FeedbackPacket : public RtcpPacket {
 public:
  /* Struct for RTP Feedback message. */
  struct Header {
    uint32_t senderSsrc;
    uint32_t mediaSsrc;
  };

 public:
  static Type rtcpType;
  static std::shared_ptr<FeedbackPacket<T>> Parse(const uint8_t* data,
                                                  size_t len);
  static const std::string& MessageType2String(typename T::MessageType type);

 private:
  static std::map<typename T::MessageType, std::string> type2String;

 public:
  typename T::MessageType GetMessageType() const { return this->messageType; }
  uint32_t GetSenderSsrc() const {
    return uint32_t{ntohl(this->header->senderSsrc)};
  }
  void SetSenderSsrc(uint32_t ssrc) {
    this->header->senderSsrc = uint32_t{htonl(ssrc)};
  }
  uint32_t GetMediaSsrc() const {
    return uint32_t{ntohl(this->header->mediaSsrc)};
  }
  void SetMediaSsrc(uint32_t ssrc) {
    this->header->mediaSsrc = uint32_t{htonl(ssrc)};
  }

  /* Pure virtual methods inherited from Packet. */
 public:
  void Dump() const override;
  size_t Serialize(uint8_t* buffer) override;
  size_t GetCount() const override {
    return static_cast<size_t>(GetMessageType());
  }
  size_t GetSize() const override {
    return sizeof(CommonHeader) + sizeof(Header);
  }

 protected:
  explicit FeedbackPacket(CommonHeader* commonHeader);
  FeedbackPacket(typename T::MessageType messageType, uint32_t senderSsrc,
                 uint32_t mediaSsrc);
  ~FeedbackPacket() override;

 private:
  Header* header{nullptr};
  uint8_t* raw{nullptr};
  typename T::MessageType messageType;
};

class FeedbackPs {
 public:
  enum class MessageType : uint8_t {
    PLI = 1,
    SLI = 2,
    RPSI = 3,
    FIR = 4,
    TSTR = 5,
    TSTN = 6,
    VBCM = 7,
    PSLEI = 8,
    ROI = 9,
    AFB = 15,
    EXT = 31
  };
};

class FeedbackRtp {
 public:
  enum class MessageType : uint8_t {
    NACK = 1,
    TMMBR = 3,
    TMMBN = 4,
    SR_REQ = 5,
    RAMS = 6,
    TLLEI = 7,
    ECN = 8,
    PS = 9,
    TCC = 15,
    QUICFB = 16,
    EXT = 31
  };
};
using FeedbackPsPacket = FeedbackPacket<FeedbackPs>;
using FeedbackRtpPacket = FeedbackPacket<FeedbackRtp>;
}  // namespace bifrost

#endif  // WORKER_RTCP_FEEDBACK_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 11:11 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_nack.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_NACK_H
#define WORKER_RTCP_NACK_H

#include "common.h"
#include "utils.h"
#include "rtcp_feedback_item.h"
#include "rtcp_feedback_rtp.h"

namespace bifrost {
class FeedbackRtpNackItem : public FeedbackItem {
 public:
  struct Header {
    uint16_t packetId;
    uint16_t lostPacketBitmask;
  };

 public:
  static const FeedbackRtp::MessageType messageType{
      FeedbackRtp::MessageType::NACK};

 public:
  explicit FeedbackRtpNackItem(Header* header) : header(header) {}
  explicit FeedbackRtpNackItem(FeedbackRtpNackItem* item)
      : header(item->header) {}
  FeedbackRtpNackItem(uint16_t packetId, uint16_t lostPacketBitmask);
  ~FeedbackRtpNackItem() override = default;

  uint16_t GetPacketId() const {
    return uint16_t{ntohs(this->header->packetId)};
  }
  uint16_t GetLostPacketBitmask() const {
    return uint16_t{ntohs(this->header->lostPacketBitmask)};
  }
  size_t CountRequestedPackets() const {
    return Bits::CountSetBits(this->header->lostPacketBitmask) + 1;
  }

  /* Virtual methods inherited from FeedbackItem. */
 public:
  void Dump() const override;
  size_t Serialize(uint8_t* buffer) override;
  size_t GetSize() const override { return sizeof(Header); }

 private:
  Header* header{nullptr};
};

// Nack packet declaration.
using FeedbackRtpNackPacket = FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
}  // namespace bifrost

#endif  // WORKER_RTCP_NACK_H

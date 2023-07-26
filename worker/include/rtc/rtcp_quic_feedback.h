/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/25 5:36 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : rtcp_quic_feedback.h
 * @description : TODO
 *******************************************************/

#ifndef _RTCP_QUIC_FEEDBACK_H
#define _RTCP_QUIC_FEEDBACK_H

/* 自定义的 quic ack

   0               1               2               3
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |V=2|P|  FMT=16 |    PT=205     |           length              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     SSRC of packet sender                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      SSRC of media source                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |              sequence         |             delta             |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          recv_bytes                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                            ......                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#include "common.h"
#include "rtcp_feedback_item.h"
#include "rtcp_feedback_rtp.h"
#include "utils.h"

namespace bifrost {
class QuicAckFeedbackItem : public FeedbackItem {
 public:
  struct Header {
    uint16_t sequence;
    uint16_t delta;
    uint32_t recv_bytes;
  };

 public:
  static const FeedbackRtp::MessageType messageType{
    FeedbackRtp::MessageType::QUICFB};

 public:
  explicit QuicAckFeedbackItem(Header* header) : header(header) {}
  explicit QuicAckFeedbackItem(QuicAckFeedbackItem* item)
  : header(item->header) {}
  QuicAckFeedbackItem(uint16_t sequence, uint16_t delta, uint32_t bytes);
  ~QuicAckFeedbackItem() override = default;

  uint16_t GetSequence() const {
    return uint16_t{ntohs(this->header->sequence)};
  }
  uint16_t GetDelta() const {
    return uint16_t{ntohs(this->header->delta)};
  }
  uint32_t GetRecvBytes() const {
    return uint32_t{ntohs(this->header->recv_bytes)};
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
using QuicAckFeedbackPacket = FeedbackRtpItemsPacket<QuicAckFeedbackItem>;
}  // namespace bifrost


#endif  //_RTCP_QUIC_FEEDBACK_H

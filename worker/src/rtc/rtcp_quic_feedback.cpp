/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/25 5:37 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : rtcp_quic_feedback.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_quic_feedback.h"

namespace bifrost {
/* Instance methods. */
QuicAckFeedbackItem::QuicAckFeedbackItem(uint16_t sequence, uint16_t delta,
                                         uint32_t bytes) {
  this->raw = new uint8_t[sizeof(Header)];
  this->header = reinterpret_cast<Header*>(this->raw);

  this->header->sequence = uint16_t{htons(sequence)};
  this->header->delta = uint16_t{htons(delta)};
  this->header->recv_bytes = uint32_t{htonl(bytes)};
}

size_t QuicAckFeedbackItem::Serialize(uint8_t* buffer) {
  // Add minimum header.
  std::memcpy(buffer, this->header, sizeof(Header));

  return sizeof(Header);
}

void QuicAckFeedbackItem::Dump() const {}
}  // namespace bifrost
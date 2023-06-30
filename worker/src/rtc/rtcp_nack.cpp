/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 11:11 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_nack.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_nack.h"

namespace bifrost {
/* Instance methods. */
FeedbackRtpNackItem::FeedbackRtpNackItem(uint16_t packetId,
                                         uint16_t lostPacketBitmask) {
  this->raw = new uint8_t[sizeof(Header)];
  this->header = reinterpret_cast<Header*>(this->raw);

  this->header->packetId = uint16_t{htons(packetId)};
  this->header->lostPacketBitmask = uint16_t{htons(lostPacketBitmask)};
}

size_t FeedbackRtpNackItem::Serialize(uint8_t* buffer) {
  // Add minimum header.
  std::memcpy(buffer, this->header, sizeof(Header));

  return sizeof(Header);
}

void FeedbackRtpNackItem::Dump() const {}
}  // namespace bifrost
/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 11:40 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_feedback_rtp.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_feedback_rtp.h"

#include "rtcp_feedback_item.h"
#include "rtcp_nack.h"

namespace bifrost {
/* Class methods. */

template <typename Item>
std::shared_ptr<FeedbackRtpItemsPacket<Item>>
FeedbackRtpItemsPacket<Item>::Parse(const uint8_t* data, size_t len) {
  if (len < sizeof(CommonHeader) + sizeof(FeedbackPacket::Header)) {
    std::cout << "[feedback rtp items] not enough space for Feedback packet, "
                 "discarded"
              << std::endl;

    return nullptr;
  }

  // NOLINTNEXTLINE(llvm-qualified-auto)
  auto* commonHeader =
      const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

  std::shared_ptr<FeedbackRtpItemsPacket<Item>> packet =
      std::make_shared<FeedbackRtpItemsPacket<Item>>(commonHeader);

  size_t offset = sizeof(CommonHeader) + sizeof(FeedbackPacket::Header);

  while (len > offset) {
    auto* item = FeedbackItem::Parse<Item>(data + offset, len - offset);

    if (item) {
      packet->AddItem(item);
      offset += item->GetSize();
    } else {
      break;
    }
  }

  return packet;
}

/* Instance methods. */

template <typename Item>
size_t FeedbackRtpItemsPacket<Item>::Serialize(uint8_t* buffer) {
  size_t offset = FeedbackPacket::Serialize(buffer);

  for (auto* item : this->items) {
    offset += item->Serialize(buffer + offset);
  }

  return offset;
}

template <typename Item>
void FeedbackRtpItemsPacket<Item>::Dump() const {}

// Explicit instantiation to have all FeedbackRtpPacket definitions in this
// file.
template class FeedbackRtpItemsPacket<FeedbackRtpNackItem>;
}  // namespace bifrost
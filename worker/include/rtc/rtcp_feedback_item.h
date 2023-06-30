/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 11:31 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_feedback_item.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_FEEDBACK_ITEM_H
#define WORKER_RTCP_FEEDBACK_ITEM_H

#include "common.h"

namespace bifrost {
class FeedbackItem {
 public:
  template <typename Item>
  static Item* Parse(const uint8_t* data, size_t len) {
    // data size must be >= header.
    if (sizeof(typename Item::Header) > len) return nullptr;

    auto* header = const_cast<typename Item::Header*>(
        reinterpret_cast<const typename Item::Header*>(data));

    return new Item(header);
  }

 public:
  bool IsCorrect() const { return this->isCorrect; }

 protected:
  virtual ~FeedbackItem() { delete[] this->raw; }

 public:
  virtual void Dump() const = 0;
  virtual void Serialize() {
    delete[] this->raw;

    this->raw = new uint8_t[this->GetSize()];
    this->Serialize(this->raw);
  }
  virtual size_t Serialize(uint8_t* buffer) = 0;
  virtual size_t GetSize() const = 0;

 protected:
  uint8_t* raw{nullptr};
  bool isCorrect{true};
};
}  // namespace bifrost

#endif  // WORKER_RTCP_FEEDBACK_ITEM_H

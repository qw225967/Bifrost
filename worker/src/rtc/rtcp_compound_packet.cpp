/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/11 11:16 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_compound_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_compound_packet.h"
#include "rtcp_packet.h"

namespace bifrost {
/* Instance methods. */

void CompoundPacket::Serialize(uint8_t* data) {
  this->header = data;

  // Calculate the total required size for the entire message.
  if (this->HasSenderReport()) {
    this->size = this->sender_report_packet_.GetSize();

    if (this->receiver_report_packet_.GetCount() != 0u) {
      this->size += sizeof(ReceiverReport::Header) *
                    this->receiver_report_packet_.GetCount();
    }
  }
  // If no sender nor receiver reports are present send an empty Receiver Report
  // packet as the head of the compound packet.
  else {
    this->size = this->receiver_report_packet_.GetSize();
  }

  // Fill it.
  size_t offset{0};

  if (this->HasSenderReport()) {
    this->sender_report_packet_.Serialize(this->header);
    offset = this->sender_report_packet_.GetSize();

    // Fix header count field.
    auto* header = reinterpret_cast<RtcpPacket::CommonHeader*>(this->header);

    header->count = 0;

    if (this->receiver_report_packet_.GetCount() != 0u) {
      // Fix header length field.
      size_t length = ((sizeof(SenderReport::Header) +
                        (sizeof(ReceiverReport::Header) *
                         this->receiver_report_packet_.GetCount())) /
                       4);

      header->length = uint16_t{htons(length)};

      // Fix header count field.
      header->count = this->receiver_report_packet_.GetCount();

      auto it = this->receiver_report_packet_.Begin();

      for (; it != this->receiver_report_packet_.End(); ++it) {
        ReceiverReport* report = (*it);

        report->Serialize(this->header + offset);
        offset += sizeof(ReceiverReport::Header);
      }
    }
  } else {
    this->receiver_report_packet_.Serialize(this->header);
    offset = this->receiver_report_packet_.GetSize();
  }
}

void CompoundPacket::Dump() {}

void CompoundPacket::AddSenderReport(SenderReport* report) {
  this->sender_report_packet_.AddReport(report);
}

void CompoundPacket::AddReceiverReport(ReceiverReport* report) {
  this->receiver_report_packet_.AddReport(report);
}
}  // namespace bifrost
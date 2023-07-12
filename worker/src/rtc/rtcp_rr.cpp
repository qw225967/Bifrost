/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/3 11:16 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_rr.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_rr.h"

namespace bifrost {
ReceiverReport* ReceiverReport::Parse(const uint8_t* data, size_t len) {
  // Get the header.
  auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

  // Packet size must be >= header size.
  if (len < sizeof(Header)) {
    std::cout << "[receive report] not enough space for receiver report, "
                 "packet discarded"
              << std::endl;

    return nullptr;
  }

  return new ReceiverReport(header);
}

/* Instance methods. */

void ReceiverReport::Dump() const {}

size_t ReceiverReport::Serialize(uint8_t* buffer) {
  // Copy the header.
  std::memcpy(buffer, this->header, sizeof(Header));

  return sizeof(Header);
}

/* Class methods. */

/**
 * ReceiverReportPacket::Parse()
 * @param  data   - Points to the begining of the incoming RTCP packet.
 * @param  len    - Total length of the packet.
 * @param  offset - points to the first Receiver Report in the incoming packet.
 */
std::shared_ptr<ReceiverReportPacket> ReceiverReportPacket::Parse(
    const uint8_t* data, size_t len, size_t offset) {
  // Get the header.
  auto* header =
      const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

  // Ensure there is space for the common header and the SSRC of packet sender.
  if (len < sizeof(CommonHeader) + 4u /* ssrc */) {
    std::cout << "[receive report] not enough space for receiver report "
                 "packet, packet discarded"
              << std::endl;

    return nullptr;
  }

  std::shared_ptr<ReceiverReportPacket> packet =
      std::make_shared<ReceiverReportPacket>(header);

  uint32_t ssrc = Byte::get_4_bytes(reinterpret_cast<uint8_t*>(header),
                                    sizeof(CommonHeader));

  packet->SetSsrc(ssrc);

  if (offset == 0) offset = sizeof(RtcpPacket::CommonHeader) + 4u /* ssrc */;

  uint8_t count = header->count;

  while ((count-- != 0u) && (len > offset)) {
    ReceiverReport* report = ReceiverReport::Parse(data + offset, len - offset);

    if (report != nullptr) {
      packet->AddReport(report);
      offset += report->GetSize();
    } else {
      return packet;
    }
  }

  return packet;
}

/* Instance methods. */

size_t ReceiverReportPacket::Serialize(uint8_t* buffer) {
  size_t offset = RtcpPacket::Serialize(buffer);

  // Copy the SSRC.
  Byte::set_4_bytes(buffer, sizeof(RtcpPacket::CommonHeader), this->ssrc);
  offset += 4u;

  // Serialize reports.
  for (auto* report : this->reports) {
    offset += report->Serialize(buffer + offset);
  }

  return offset;
}

void ReceiverReportPacket::Dump() const {}
}  // namespace bifrost
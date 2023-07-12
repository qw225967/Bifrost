/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/3 11:14 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_sr.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_sr.h"

namespace bifrost {
/* Class methods. */

SenderReport* SenderReport::Parse(const uint8_t* data, size_t len) {
  // Get the header.
  auto* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

  // Packet size must be >= header size.
  if (len < sizeof(Header)) {
    std::cout
        << "[send report] not enough space for sender report, packet discarded"
        << std::endl;

    return nullptr;
  }

  return new SenderReport(header);
}

/* Instance methods. */

void SenderReport::Dump() const {}

size_t SenderReport::Serialize(uint8_t* buffer) {
  // Copy the header.
  std::memcpy(buffer, this->header, sizeof(Header));

  return sizeof(Header);
}

/* Class methods. */

std::shared_ptr<SenderReportPacket> SenderReportPacket::Parse(
    const uint8_t* data, size_t len) {
  // Get the header.
  auto* header =
      const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

  std::shared_ptr<SenderReportPacket> packet =
      std::make_shared<SenderReportPacket>(header);
  size_t offset = sizeof(RtcpPacket::CommonHeader);

  SenderReport* report = SenderReport::Parse(data + offset, len - offset);

  if (report) packet->AddReport(report);

  return packet;
}

/* Instance methods. */

size_t SenderReportPacket::Serialize(uint8_t* buffer) {
  size_t offset = RtcpPacket::Serialize(buffer);

  // Serialize reports.
  for (auto* report : this->reports) {
    offset += report->Serialize(buffer + offset);
  }

  return offset;
}

void SenderReportPacket::Dump() const {}
}  // namespace bifrost
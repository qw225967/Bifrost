/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:35 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_tcc.cpp
 * @description : TODO
 *******************************************************/

#include "rtcp_tcc.h"

#include <limits>  // std::numeric_limits()
#include <sstream>

#include "sequence_manager.h"
#include "utils.h"

// Code taken and adapted from libwebrtc (byte_io.h).
inline static int32_t parseReferenceTime(uint8_t* buffer) {
  int32_t referenceTime;
  uint32_t unsignedVal = bifrost::Byte::get_3_bytes(buffer, 0);

  const auto msb = static_cast<uint8_t>(unsignedVal >> ((3 - 1) * 8));

  if ((msb & 0x80) != 0) {
    // Create a mask where all bits used by the 3 bytes are set to one, for
    // instance 0x00FFFFFF. Bit-wise inverts that mask to 0xFF000000 and adds
    // it to the input value.
    static uint32_t usedBitMask = (1 << ((3 % sizeof(int32_t)) * 8)) - 1;

    unsignedVal = ~usedBitMask | unsignedVal;
  }

  // An unsigned value with only the highest order bit set (ex 0x80).
  static uint32_t unsignedHighestBitMask = static_cast<uint32_t>(1)
                                           << ((sizeof(uint32_t) * 8) - 1);

  // A signed value with only the highest bit set. Since this is two's
  // complement form, we can use the min value from std::numeric_limits.
  static int32_t signedHighestBitMask = std::numeric_limits<int32_t>::min();

  if ((unsignedVal & unsignedHighestBitMask) != 0) {
    // Casting is only safe when unsigned value can be represented in the
    // signed target type, so mask out highest bit and mask it back manually.
    referenceTime = static_cast<int32_t>(unsignedVal & ~unsignedHighestBitMask);
    referenceTime |= signedHighestBitMask;
  } else {
    referenceTime = static_cast<int32_t>(unsignedVal);
  }

  return referenceTime;
}

namespace bifrost {
/* Static members. */

size_t FeedbackRtpTransportPacket::fixedHeaderSize{8u};
uint16_t FeedbackRtpTransportPacket::maxMissingPackets{(1 << 13) - 1};
uint16_t FeedbackRtpTransportPacket::maxPacketStatusCount{(1 << 16) - 1};
int16_t FeedbackRtpTransportPacket::maxPacketDelta{0x7FFF};

// clang-format off
std::map<FeedbackRtpTransportPacket::Status, std::string> FeedbackRtpTransportPacket::status2String =
    {
    { FeedbackRtpTransportPacket::Status::NotReceived, "NR" },
    { FeedbackRtpTransportPacket::Status::SmallDelta,  "SD" },
    { FeedbackRtpTransportPacket::Status::LargeDelta,  "LD" }
    };
// clang-format on

/* Class methods. */

FeedbackRtpTransportPacket* FeedbackRtpTransportPacket::Parse(
    const uint8_t* data, size_t len) {
  if (len < sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) +
                FeedbackRtpTransportPacket::fixedHeaderSize) {
    std::cout << "[feedback tcc packet] not enough space for Feedback packet, "
                 "discarded"
              << std::endl;

    return nullptr;
  }

  // NOLINTNEXTLINE(llvm-qualified-auto)
  auto* commonHeader =
      const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

  std::unique_ptr<FeedbackRtpTransportPacket> packet(
      new FeedbackRtpTransportPacket(commonHeader, len));

  if (!packet->IsCorrect()) return nullptr;

  return packet.release();
}

/* Instance methods. */

FeedbackRtpTransportPacket::FeedbackRtpTransportPacket(
    CommonHeader* commonHeader, size_t availableLen)
    : FeedbackRtpPacket(commonHeader) {
  size_t len = static_cast<size_t>(ntohs(commonHeader->length) + 1) * 4;

  if (len > availableLen) {
    std::cout << "[feedback tcc packet] packet announced length exceeds the "
                 "available buffer length, "
                 "discarded"
              << std::endl;

    this->isCorrect = false;

    return;
  }

  // Make data point to the packet specific info.
  auto* data = reinterpret_cast<uint8_t*>(commonHeader) + sizeof(CommonHeader) +
               sizeof(FeedbackPacket::Header);

  this->baseSequenceNumber = Byte::get_2_bytes(data, 0);
  this->packetStatusCount = Byte::get_2_bytes(data, 2);
  this->referenceTime = parseReferenceTime(data + 4);
  this->feedbackPacketCount = Byte::get_1_byte(data, 7);
  this->size = len;

  // Make contentData point to the beginning of the chunks.
  uint8_t* contentData = data + FeedbackRtpTransportPacket::fixedHeaderSize;
  // Make contentLen be the available length for chunks.
  size_t contentLen = len - sizeof(CommonHeader) -
                      sizeof(FeedbackPacket::Header) -
                      FeedbackRtpTransportPacket::fixedHeaderSize;
  size_t offset{0u};
  uint16_t count{0u};
  uint16_t receivedPacketStatusCount{0u};

  while (count < this->packetStatusCount && contentLen > offset) {
    if (contentLen - offset < 2u) {
      std::cout << "[feedback tcc packet] not enough space for chunk"
                << std::endl;

      this->isCorrect = false;

      return;
    }

    auto* chunk = Chunk::Parse(contentData + offset, contentLen - offset,
                               this->packetStatusCount - count);

    if (!chunk) {
      std::cout << "[feedback tcc packet] invalid chunk" << std::endl;

      this->isCorrect = false;

      return;
    }

    this->chunks.push_back(chunk);
    this->deltasAndChunksSize += 2u;

    offset += 2u;
    count += chunk->GetCount();
    receivedPacketStatusCount += chunk->GetReceivedStatusCount();
  }

  if (count != this->packetStatusCount) {
    std::cout << "[feedback tcc packet] provided packet status count does not "
                 "match with content"
              << std::endl;

    this->isCorrect = false;

    return;
  }

  auto chunksIt = this->chunks.begin();

  while (chunksIt != this->chunks.end() && contentLen > offset) {
    size_t deltasOffset{0u};
    auto* chunk = *chunksIt;

    if (!chunk->AddDeltas(contentData + offset, contentLen - offset,
                          this->deltas, deltasOffset)) {
      std::cout << "[feedback tcc packet] not enough space for deltas"
                << std::endl;

      this->isCorrect = false;

      return;
    }

    offset += deltasOffset;
    this->deltasAndChunksSize += deltasOffset;

    ++chunksIt;
  }

  if (this->deltas.size() != receivedPacketStatusCount) {
    std::cout << "[feedback tcc packet] received deltas does not match "
                 "received status count"
              << std::endl;

    this->isCorrect = false;

    return;
  }
}

FeedbackRtpTransportPacket::~FeedbackRtpTransportPacket() {
  for (auto* chunk : this->chunks) {
    delete chunk;
  }
  this->chunks.clear();
}

void FeedbackRtpTransportPacket::Dump() const {}

size_t FeedbackRtpTransportPacket::Serialize(uint8_t* buffer) {
  // Add chunks for status packets that may not be represented yet.
  AddPendingChunks();

  size_t offset = FeedbackPacket::Serialize(buffer);

  // Base sequence number.
  Byte::set_2_bytes(buffer, offset, this->baseSequenceNumber);
  offset += 2;

  // Packet status count.
  Byte::set_2_bytes(buffer, offset, this->packetStatusCount);
  offset += 2;

  // Reference time.
  Byte::set_3_bytes(buffer, offset, static_cast<uint32_t>(this->referenceTime));
  offset += 3;

  // Feedback packet count.
  Byte::set_1_byte(buffer, offset, this->feedbackPacketCount);
  offset += 1;

  // Serialize chunks.
  for (auto* chunk : this->chunks) {
    offset += chunk->Serialize(buffer + offset);
  }

  // Serialize deltas.
  for (auto delta : this->deltas) {
    if (delta >= 0 && delta <= 255) {
      Byte::set_1_byte(buffer, offset, delta);
      offset += 1u;
    } else {
      Byte::set_2_bytes(buffer, offset, delta);
      offset += 2u;
    }
  }

  // 32 bits padding.
  size_t padding = (-offset) & 3;

  for (size_t i{0u}; i < padding; ++i) {
    buffer[offset + i] = 0u;
  }

  offset += padding;

  return offset;
}

FeedbackRtpTransportPacket::AddPacketResult
FeedbackRtpTransportPacket::AddPacket(uint16_t sequenceNumber,
                                      uint64_t timestamp,
                                      size_t maxRtcpPacketLen) {
  // Let's see if we must set our base.
  if (this->latestTimestamp == 0u) {
    this->baseSequenceNumber = sequenceNumber + 1;
    this->referenceTime = static_cast<int32_t>((timestamp & 0x1FFFFFC0) / 64);
    this->latestSequenceNumber = sequenceNumber;
    this->latestTimestamp =
        (timestamp >> 6) * 64;  // IMPORTANT: Loose precision.

    return AddPacketResult::SUCCESS;
  }

  // If the wide sequence number of the new packet is lower than the latest
  // seen, ignore it. NOTE: Not very spec compliant but libwebrtc does it. Also
  // ignore if the sequence number matches the latest seen.
  if (!SeqManager<uint16_t>::IsSeqHigherThan(sequenceNumber,
                                             this->latestSequenceNumber)) {
    return AddPacketResult::SUCCESS;
  }

  // Check if there are too many missing packets.
  {
    auto missingPackets = sequenceNumber - (this->latestSequenceNumber + 1);

    if (missingPackets > FeedbackRtpTransportPacket::maxMissingPackets) {
      std::cout << "[feedback tcc packet] RTP missing packet number exceeded"
                << std::endl;

      return AddPacketResult::FATAL;
    }
  }

  // Deltas are represented as multiples of 250 us.
  // NOTE: Read it as int 64 to detect long elapsed times.
  int64_t delta64 = (timestamp - this->latestTimestamp) * 4;

  // clang-format off
  if (
      delta64 > FeedbackRtpTransportPacket::maxPacketDelta ||
      delta64 < -1 * static_cast<int64_t>(FeedbackRtpTransportPacket::maxPacketDelta)
      )
  // clang-format on
  {
    std::cout
        << "[feedback tcc packet] RTP packet delta exceeded [latestTimestamp:"
        << this->latestTimestamp << ", timestamp:%" << timestamp << "]"
        << std::endl;

    return AddPacketResult::FATAL;
  }

  // Delta in 16 bits signed.
  auto delta = static_cast<int16_t>(delta64);

  // Check whether another chunks and corresponding delta infos could be added.
  {
    // Fixed packet size.
    size_t size = FeedbackRtpPacket::GetSize();

    size += FeedbackRtpTransportPacket::fixedHeaderSize;
    size += this->deltasAndChunksSize;

    // Maximum size needed for another chunk and its delta infos.
    size += 2u;
    size += 2u;

    // 32 bits padding.
    size += (-size) & 3;

    if (size > maxRtcpPacketLen) {
      std::cout << "[feedback tcc packet] maximum packet size exceeded"
                << std::endl;

      return AddPacketResult::MAX_SIZE_EXCEEDED;
    }
  }

  // Fill a chunk.
  FillChunk(this->latestSequenceNumber, sequenceNumber, delta);

  // Update latest seen sequence number and timestamp.
  this->latestSequenceNumber = sequenceNumber;
  this->latestTimestamp = timestamp;

  return AddPacketResult::SUCCESS;
}

void FeedbackRtpTransportPacket::Finish() { AddPendingChunks(); }

std::vector<struct FeedbackRtpTransportPacket::PacketResult>
FeedbackRtpTransportPacket::GetPacketResults() const {
  std::vector<struct PacketResult> packetResults;

  uint16_t currentSequenceNumber = this->baseSequenceNumber - 1;

  for (auto* chunk : this->chunks) {
    chunk->FillResults(packetResults, currentSequenceNumber);
  }

  size_t deltaIdx{0u};
  int64_t currentReceivedAtMs = static_cast<int64_t>(this->referenceTime * 64);

  for (size_t idx{0u}; idx < packetResults.size(); ++idx) {
    auto& packetResult = packetResults[idx];

    if (!packetResult.received) continue;

    currentReceivedAtMs += this->deltas.at(deltaIdx) / 4;
    packetResult.delta = this->deltas.at(deltaIdx);
    packetResult.receivedAtMs = currentReceivedAtMs;
    deltaIdx++;
  }

  return packetResults;
}

uint8_t FeedbackRtpTransportPacket::GetPacketFractionLost() const {
  uint16_t expected = this->packetStatusCount;
  uint16_t lost{0u};

  if (expected == 0u) return 0u;

  for (auto* chunk : this->chunks) {
    lost += chunk->GetCount() - chunk->GetReceivedStatusCount();
  }

  // NOTE: If lost equals expected, the math below would produce 256, which
  // becomes 0 in uint8_t.
  if (lost == expected) return 255u;

  return (lost << 8) / expected;
}

void FeedbackRtpTransportPacket::FillChunk(uint16_t previousSequenceNumber,
                                           uint16_t sequenceNumber,
                                           int16_t delta) {
  auto missingPackets =
      static_cast<uint16_t>(sequenceNumber - (previousSequenceNumber + 1));

  if (missingPackets > 0) {
    // Create a long run chunk before processing this packet, if needed.
    if (this->context.statuses.size() >= 7 && this->context.allSameStatus) {
      CreateRunLengthChunk(this->context.currentStatus,
                           this->context.statuses.size());

      this->context.statuses.clear();
      this->context.currentStatus = Status::None;
    }

    this->context.currentStatus = Status::NotReceived;
    size_t representedPackets{0u};

    // Fill statuses vector.
    for (uint8_t i{0u}; i < missingPackets && this->context.statuses.size() < 7;
         ++i) {
      this->context.statuses.emplace_back(Status::NotReceived);
      representedPackets++;
    }

    // Create a two bit vector if needed.
    if (this->context.statuses.size() == 7) {
      // Fill a vector chunk.
      CreateTwoBitVectorChunk(this->context.statuses);

      this->context.statuses.clear();
      this->context.currentStatus = Status::None;
    }

    missingPackets -= representedPackets;

    // Not all missing packets have been represented.
    if (missingPackets != 0) {
      // Fill a run length chunk with the remaining missing packets.
      CreateRunLengthChunk(Status::NotReceived, missingPackets);

      this->context.statuses.clear();
      this->context.currentStatus = Status::None;
    }
  }

  Status status;

  if (delta >= 0 && delta <= 255)
    status = Status::SmallDelta;
  else
    status = Status::LargeDelta;

  // Create a long run chunk before processing this packet, if needed.
  // clang-format off
  if (
      this->context.statuses.size() >= 7 &&
      this->context.allSameStatus &&
      status != this->context.currentStatus
      )
  // clang-format on
  {
    CreateRunLengthChunk(this->context.currentStatus,
                         this->context.statuses.size());

    this->context.statuses.clear();
  }

  this->context.statuses.emplace_back(status);
  this->deltas.push_back(delta);
  this->deltasAndChunksSize += (status == Status::SmallDelta) ? 1u : 2u;

  // Update context info.

  // clang-format off
  if (
      this->context.currentStatus == Status::None ||
      (this->context.allSameStatus && this->context.currentStatus == status)
      )
  // clang-format on
  {
    this->context.allSameStatus = true;
  } else {
    this->context.allSameStatus = false;
  }

  this->context.currentStatus = status;

  // Not enough packet infos for creating a chunk.
  if (this->context.statuses.size() < 7) {
    return;
  }
  // 7 packet infos with heterogeneous status, create the chunk.
  else if (this->context.statuses.size() == 7 && !this->context.allSameStatus) {
    // Reset current status.
    this->context.currentStatus = Status::None;

    // Fill a vector chunk and return.
    CreateTwoBitVectorChunk(this->context.statuses);

    this->context.statuses.clear();
  }
}

void FeedbackRtpTransportPacket::CreateRunLengthChunk(Status status,
                                                      uint16_t count) {
  auto* chunk = new RunLengthChunk(status, count);

  this->chunks.push_back(chunk);
  this->packetStatusCount += count;
  this->deltasAndChunksSize += 2u;
}

void FeedbackRtpTransportPacket::CreateOneBitVectorChunk(
    std::vector<Status>& statuses) {
  auto* chunk = new OneBitVectorChunk(statuses);

  this->chunks.push_back(chunk);
  this->packetStatusCount += static_cast<uint16_t>(statuses.size());
  this->deltasAndChunksSize += 2u;
}

void FeedbackRtpTransportPacket::CreateTwoBitVectorChunk(
    std::vector<Status>& statuses) {
  auto* chunk = new TwoBitVectorChunk(statuses);

  this->chunks.push_back(chunk);
  this->packetStatusCount += static_cast<uint16_t>(statuses.size());
  this->deltasAndChunksSize += 2u;
}

void FeedbackRtpTransportPacket::AddPendingChunks() {
  // No pending status packets.
  if (this->context.statuses.empty()) return;

  if (this->context.allSameStatus) {
    CreateRunLengthChunk(this->context.currentStatus,
                         this->context.statuses.size());
  } else {
    CreateTwoBitVectorChunk(this->context.statuses);
  }

  this->context.statuses.clear();
}

FeedbackRtpTransportPacket::Chunk* FeedbackRtpTransportPacket::Chunk::Parse(
    const uint8_t* data, size_t len, uint16_t count) {
  if (len < 2u) {
    std::cout << "[feedback tcc packet] not enough space for "
                 "FeedbackRtpTransportPacket chunk, discarded"
              << std::endl;

    return nullptr;
  }

  auto bytes = Byte::get_2_bytes(data, 0);
  uint8_t chunkType = (bytes >> 15) & 0x01;

  // Run length chunk.
  if (chunkType == 0) {
    auto* chunk = new RunLengthChunk(bytes);

    // Verify that the status is a valid one.
    switch (chunk->GetStatus()) {
      case Status::NotReceived:
      case Status::SmallDelta:
      case Status::LargeDelta: {
        return chunk;
      }

      default: {
        std::cout
            << "[feedback tcc packet] invalid status for a run length chunk"
            << std::endl;

        delete chunk;

        return nullptr;
      }
    }
  }
  // Vector chunk.
  else {
    uint8_t symbolSize = data[0] & 0x40;

    if (symbolSize == 0)
      return new OneBitVectorChunk(bytes, count);
    else
      return new TwoBitVectorChunk(bytes, count);
  }

  return nullptr;
}

FeedbackRtpTransportPacket::RunLengthChunk::RunLengthChunk(uint16_t buffer) {
  this->status = static_cast<Status>((buffer >> 13) & 0x03);
  this->count = buffer & 0x1FFF;
}

bool FeedbackRtpTransportPacket::RunLengthChunk::AddDeltas(
    const uint8_t* data, size_t len, std::vector<int16_t>& deltas,
    size_t& offset) {
  // No delta to be added.
  if (this->status == Status::NotReceived) {
    return true;
  } else if (this->status == Status::SmallDelta) {
    if (len < this->count * 1u) {
      std::cout << "[feedback tcc packet] not enough space for small deltas"
                << std::endl;

      return false;
    }

    for (size_t i{0}; i < this->count; ++i) {
      auto delta = static_cast<int16_t>(Byte::get_1_byte(data, offset));

      deltas.push_back(delta);
      offset += 1u;
    }
  } else if (this->status == Status::LargeDelta) {
    if (len < this->count * 2u) {
      std::cout << "[feedback tcc packet] not enough space for large deltas"
                << std::endl;

      return false;
    }

    for (size_t i{0}; i < this->count; ++i) {
      auto delta = static_cast<int16_t>(Byte::get_2_bytes(data, offset));

      deltas.push_back(delta);
      offset += 2u;
    }
  }

  return true;
}

void FeedbackRtpTransportPacket::RunLengthChunk::Dump() const {}

uint16_t FeedbackRtpTransportPacket::RunLengthChunk::GetReceivedStatusCount()
    const {
  if (this->status == Status::SmallDelta || this->status == Status::LargeDelta)
    return this->count;
  else
    return 0u;
}

void FeedbackRtpTransportPacket::RunLengthChunk::FillResults(
    std::vector<struct FeedbackRtpTransportPacket::PacketResult>& packetResults,
    uint16_t& currentSequenceNumber) const {
  bool received = (this->status == Status::SmallDelta ||
                   this->status == Status::LargeDelta);

  for (uint16_t count{1u}; count <= this->count; ++count) {
    packetResults.emplace_back(++currentSequenceNumber, received);
  }
}

size_t FeedbackRtpTransportPacket::RunLengthChunk::Serialize(uint8_t* buffer) {
  uint16_t bytes{0x0000};

  bytes |= this->status << 13;
  bytes |= this->count & 0x1FFF;

  Byte::set_2_bytes(buffer, 0, bytes);

  return 2u;
}

FeedbackRtpTransportPacket::OneBitVectorChunk::OneBitVectorChunk(
    uint16_t buffer, uint16_t count) {
  for (uint8_t i{0u}; i < 14 && count > 0; ++i, --count) {
    auto status = static_cast<Status>((buffer >> (14 - 1 - i)) & 0x01);

    this->statuses.emplace_back(status);
  }
}

bool FeedbackRtpTransportPacket::OneBitVectorChunk::AddDeltas(
    const uint8_t* data, size_t len, std::vector<int16_t>& deltas,
    size_t& offset) {
  for (auto status : this->statuses) {
    if (status == Status::NotReceived) {
      continue;
    } else if (status == Status::SmallDelta) {
      if (len < 1u) {
        std::cout << "[feedback tcc packet] not enough space for small delta"
                  << std::endl;

        return false;
      }

      auto delta = static_cast<int16_t>(Byte::get_1_byte(data, offset));

      deltas.push_back(delta);
      offset += 1u;
      len -= 1u;

      continue;
    } else {
      std::cout
          << "[feedback tcc packet] invalid status for one bit vector chunk"
          << std::endl;

      return false;
    }
  }

  return true;
}

void FeedbackRtpTransportPacket::OneBitVectorChunk::Dump() const {
  std::ostringstream out;

  // Dump status slots.
  for (auto status : this->statuses) {
    out << "|" << FeedbackRtpTransportPacket::status2String[status];
  }

  // Dump empty slots.
  for (size_t i{this->statuses.size()}; i < 14; ++i) {
    out << "|--";
  }

  out << "|";
}

uint16_t FeedbackRtpTransportPacket::OneBitVectorChunk::GetReceivedStatusCount()
    const {
  uint16_t count{0u};

  for (auto status : this->statuses) {
    if (status == Status::SmallDelta || status == Status::LargeDelta) count++;
  }

  return count;
}

void FeedbackRtpTransportPacket::OneBitVectorChunk::FillResults(
    std::vector<struct FeedbackRtpTransportPacket::PacketResult>& packetResults,
    uint16_t& currentSequenceNumber) const {
  for (auto status : this->statuses) {
    bool received =
        (status == Status::SmallDelta || status == Status::LargeDelta);

    packetResults.emplace_back(++currentSequenceNumber, received);
  }
}

size_t FeedbackRtpTransportPacket::OneBitVectorChunk::Serialize(
    uint8_t* buffer) {
  uint16_t bytes{0x8000};
  uint8_t i{13u};

  for (auto status : this->statuses) {
    bytes |= status << i;
    i -= 1;
  }

  Byte::set_2_bytes(buffer, 0, bytes);

  return 2u;
}

FeedbackRtpTransportPacket::TwoBitVectorChunk::TwoBitVectorChunk(
    uint16_t buffer, uint16_t count) {
  for (uint8_t i{0u}; i < 7 && count > 0; ++i, --count) {
    auto status = static_cast<Status>((buffer >> 2 * (7 - 1 - i)) & 0x03);

    this->statuses.emplace_back(status);
  }
}

bool FeedbackRtpTransportPacket::TwoBitVectorChunk::AddDeltas(
    const uint8_t* data, size_t len, std::vector<int16_t>& deltas,
    size_t& offset) {
  for (auto status : this->statuses) {
    if (status == Status::NotReceived) {
      continue;
    } else if (status == Status::SmallDelta) {
      if (len < 1u) {
        std::cout << "[feedback tcc packet] not enough space for small delta"
                  << std::endl;

        return false;
      }

      auto delta = static_cast<int16_t>(Byte::get_1_byte(data, offset));

      deltas.push_back(delta);
      offset += 1u;
      len -= 1u;

      continue;
    } else if (status == Status::LargeDelta) {
      if (len < 2u) {
        std::cout << "[feedback tcc packet] not enough space for large delta"
                  << std::endl;

        return false;
      }

      auto delta = static_cast<int16_t>(Byte::get_2_bytes(data, offset));

      deltas.push_back(delta);
      offset += 2u;
      len -= 2u;

      continue;
    }
  }

  return true;
}

void FeedbackRtpTransportPacket::TwoBitVectorChunk::Dump() const {
  std::ostringstream out;

  // Dump status slots.
  for (auto status : this->statuses) {
    out << "|" << FeedbackRtpTransportPacket::status2String[status];
  }

  // Dump empty slots.
  for (size_t i{this->statuses.size()}; i < 7; ++i) {
    out << "|--";
  }

  out << "|";
}

uint16_t FeedbackRtpTransportPacket::TwoBitVectorChunk::GetReceivedStatusCount()
    const {
  uint16_t count{0u};

  for (auto status : this->statuses) {
    if (status == Status::SmallDelta || status == Status::LargeDelta) count++;
  }

  return count;
}

void FeedbackRtpTransportPacket::TwoBitVectorChunk::FillResults(
    std::vector<struct FeedbackRtpTransportPacket::PacketResult>& packetResults,
    uint16_t& currentSequenceNumber) const {
  for (auto status : this->statuses) {
    bool received =
        (status == Status::SmallDelta || status == Status::LargeDelta);

    packetResults.emplace_back(++currentSequenceNumber, received);
  }
}

size_t FeedbackRtpTransportPacket::TwoBitVectorChunk::Serialize(
    uint8_t* buffer) {
  uint16_t bytes{0x8000};
  uint8_t i{12u};

  bytes |= 0x01 << 14;

  for (auto status : this->statuses) {
    bytes |= status << i;
    i -= 2;
  }

  Byte::set_2_bytes(buffer, 0, bytes);

  return 2u;
}
}  // namespace bifrost
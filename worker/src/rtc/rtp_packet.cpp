/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 10:17 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtp_packet.cpp
 * @description : TODO
 *******************************************************/

#include "rtp_packet.h"

#include <cstring>  // std::memcpy(), std::memmove(), std::memset()
#include <iostream>
#include <iterator>  // std::ostream_iterator
#include <sstream>   // std::ostringstream

namespace bifrost {
/* Class methods. */

std::shared_ptr<RtpPacket> RtpPacket::Parse(const uint8_t* data, size_t len) {
  if (!RtpPacket::IsRtp(data, len)) return nullptr;

  auto* ptr = const_cast<uint8_t*>(data);

  // Get the header.
  auto* header = reinterpret_cast<Header*>(ptr);

  // Inspect data after the minimum header size.
  ptr += HeaderSize;

  // Check CSRC list.
  size_t csrcListSize{0u};

  if (header->csrcCount != 0u) {
    csrcListSize = header->csrcCount * sizeof(header->ssrc);

    // Packet size must be >= header size + CSRC list.
    if (len < (ptr - data) + csrcListSize) {
      std::cout << "[rtp packet] not enough space for the announced CSRC list, "
                   "packet discarded"
                << std::endl;

      return nullptr;
    }
    ptr += csrcListSize;
  }

  // Check header extension.
  HeaderExtension* headerExtension{nullptr};
  size_t extensionValueSize{0u};

  if (header->extension == 1u) {
    // The header extension is at least 4 bytes.
    if (len < static_cast<size_t>(ptr - data) + 4) {
      std::cout << "[rtp packet] not enough space for the announced header "
                   "extension, packet "
                   "discarded"
                << std::endl;

      return nullptr;
    }

    headerExtension = reinterpret_cast<HeaderExtension*>(ptr);

    // The header extension contains a 16-bit length field that counts the
    // number of 32-bit words in the extension, excluding the four-octet header
    // extension.
    extensionValueSize =
        static_cast<size_t>(ntohs(headerExtension->length) * 4);

    // Packet size must be >= header size + CSRC list + header extension size.
    if (len < (ptr - data) + 4 + extensionValueSize) {
      std::cout << "[rtp packet] not enough space for the announced header "
                   "extension value, "
                   "packet discarded"
                << std::endl;

      return nullptr;
    }
    ptr += 4 + extensionValueSize;
  }

  // Get payload.
  uint8_t* payload = ptr;
  size_t payloadLength = len - (ptr - data);
  uint8_t payloadPadding{0};

  // Check padding field.
  if (header->padding != 0u) {
    // Must be at least a single payload byte.
    if (payloadLength == 0) {
      std::cout << "[rtp packet] padding bit is set but no space for a padding "
                   "byte, packet "
                   "discarded"
                << std::endl;

      return nullptr;
    }

    payloadPadding = data[len - 1];
    if (payloadPadding == 0) {
      std::cout << "[rtp packet] padding byte cannot be 0, packet discarded"
                << std::endl;

      return nullptr;
    }

    if (payloadLength < size_t{payloadPadding}) {
      std::cout << "[rtp packet] number of padding octets is greater than "
                   "available space "
                   "for payload, packet "
                   "discarded"
                << std::endl;

      return nullptr;
    }
    payloadLength -= size_t{payloadPadding};
  }

  std::shared_ptr<RtpPacket> packet = std::make_shared<RtpPacket>(
      header, headerExtension, payload, payloadLength, payloadPadding, len);

  return packet;
}

/* Instance methods. */

RtpPacket::RtpPacket(Header* header, HeaderExtension* headerExtension,
                     const uint8_t* payload, size_t payloadLength,
                     uint8_t payloadPadding, size_t size)
    : header(header),
      headerExtension(headerExtension),
      payload(const_cast<uint8_t*>(payload)),
      payloadLength(payloadLength),
      payloadPadding(payloadPadding),
      size(size) {
  if (this->header->csrcCount != 0u)
    this->csrcList = reinterpret_cast<uint8_t*>(header) + HeaderSize;

  // Parse RFC 5285 header extension.
  ParseExtensions();
}

RtpPacket::~RtpPacket() {}

void RtpPacket::Dump() const {}

void RtpPacket::FillJson(json& jsonObject) const {
  // Add payloadType.
  jsonObject["payloadType"] = GetPayloadType();

  // Add sequenceNumber.
  jsonObject["sequenceNumber"] = GetSequenceNumber();

  // Add timestamp.
  jsonObject["timestamp"] = GetTimestamp();

  // Add marker.
  jsonObject["marker"] = HasMarker();

  // Add ssrc.
  jsonObject["ssrc"] = GetSsrc();

  // Add isKeyFrame.
  jsonObject["isKeyFrame"] = IsKeyFrame();

  // Add size.
  jsonObject["size"] = GetSize();

  // Add payloadSize.
  jsonObject["payloadSize"] = GetPayloadLength();

  // Add spatialLayer.
  jsonObject["spatialLayer"] = GetSpatialLayer();

  // Add temporalLayer.
  jsonObject["temporalLayer"] = GetTemporalLayer();

  // Add mid.
  std::string mid;

  if (this->midExtensionId != 0u && ReadMid(mid)) jsonObject["mid"] = mid;

  // Add rid.
  std::string rid;

  if (this->ridExtensionId != 0u && ReadRid(rid)) jsonObject["rid"] = rid;

  // Add rrid.
  std::string rrid;

  if (this->rridExtensionId != 0u && ReadRid(rrid)) jsonObject["rrid"] = rrid;

  // Add wideSequenceNumber.
  uint16_t wideSequenceNumber;

  if (this->transportWideCc01ExtensionId != 0u &&
      ReadTransportWideCc01(wideSequenceNumber))
    jsonObject["wideSequenceNumber"] = wideSequenceNumber;
}

void RtpPacket::SetExtensions(uint8_t type,
                              const std::vector<GenericExtension>& extensions) {
}

bool RtpPacket::UpdateMid(const std::string& mid) {
  uint8_t extenLen;
  uint8_t* extenValue = GetExtension(this->midExtensionId, extenLen);

  if (!extenValue) return false;

  // Here we assume that there is MidMaxLength available bytes, even if now
  // they are padding bytes.
  if (mid.size() > MidMaxLength) {
    std::cout << "[rtp packet] no enough space for MID value [MidMaxLength:"
              << MidMaxLength << ", mid:'" << mid.c_str() << "']" << std::endl;
    return false;
  }

  std::memcpy(extenValue, mid.c_str(), mid.size());

  SetExtensionLength(this->midExtensionId, mid.size());

  return true;
}

/**
 * The caller is responsible of not setting a length higher than the
 * available one (taking into account existing padding bytes).
 */
bool RtpPacket::SetExtensionLength(uint8_t id, uint8_t len) {}

void RtpPacket::SetPayloadLength(size_t length) {
  // Pad desired length to 4 bytes.
  length =
      static_cast<size_t>(Byte::PadTo4Bytes(static_cast<uint16_t>(length)));

  this->size -= this->payloadLength;
  this->size -= size_t{this->payloadPadding};
  this->payloadLength = length;
  this->payloadPadding = 0u;
  this->size += length;

  SetPayloadPaddingFlag(false);
}

RtpPacket* RtpPacket::Clone(const uint8_t* buffer) const {
  auto* ptr = const_cast<uint8_t*>(buffer);
  size_t numBytes{0};

  // Copy the minimum header.
  numBytes = HeaderSize;
  std::memcpy(ptr, GetData(), numBytes);

  // Set header pointer.
  auto* newHeader = reinterpret_cast<Header*>(ptr);

  ptr += numBytes;

  // Copy CSRC list.
  if (this->csrcList != nullptr) {
    numBytes = this->header->csrcCount * sizeof(this->header->ssrc);
    std::memcpy(ptr, this->csrcList, numBytes);

    ptr += numBytes;
  }

  // Copy header extension.
  HeaderExtension* newHeaderExtension{nullptr};

  if (this->headerExtension != nullptr) {
    numBytes = 4 + GetHeaderExtensionLength();
    std::memcpy(ptr, this->headerExtension, numBytes);

    // Set the header extension pointer.
    newHeaderExtension = reinterpret_cast<HeaderExtension*>(ptr);

    ptr += numBytes;
  }

  // Copy payload.
  uint8_t* newPayload{ptr};

  if (this->payloadLength != 0u) {
    numBytes = this->payloadLength;
    std::memcpy(ptr, this->payload, numBytes);

    ptr += numBytes;
  }

  // Copy payload padding.
  if (this->payloadPadding != 0u) {
    *(ptr + static_cast<size_t>(this->payloadPadding) - 1) =
        this->payloadPadding;
    ptr += size_t{this->payloadPadding};
  }

  // Create the new RtpPacket instance and return it.
  auto* packet =
      new RtpPacket(newHeader, newHeaderExtension, newPayload,
                    this->payloadLength, this->payloadPadding, this->size);

  // Keep already set extension ids.
  packet->midExtensionId = this->midExtensionId;
  packet->ridExtensionId = this->ridExtensionId;
  packet->rridExtensionId = this->rridExtensionId;
  packet->absSendTimeExtensionId = this->absSendTimeExtensionId;
  packet->transportWideCc01ExtensionId = this->transportWideCc01ExtensionId;
  packet->frameMarking07ExtensionId =
      this->frameMarking07ExtensionId;  // Remove once RFC.
  packet->frameMarkingExtensionId = this->frameMarkingExtensionId;
  packet->ssrcAudioLevelExtensionId = this->ssrcAudioLevelExtensionId;
  packet->videoOrientationExtensionId = this->videoOrientationExtensionId;

  return packet;
}

// NOTE: The caller must ensure that the buffer/memmory of the packet has
// space enough for adding 2 extra bytes.
void RtpPacket::RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq) {
  // Rewrite the payload type.
  SetPayloadType(payloadType);

  // Rewrite the SSRC.
  SetSsrc(ssrc);

  // Write the original sequence number at the begining of the payload.
  std::memmove(this->payload + 2, this->payload, this->payloadLength);
  Byte::set_2_byte(this->payload, 0, GetSequenceNumber());

  // Rewrite the sequence number.
  SetSequenceNumber(seq);

  // Fix the payload length.
  this->payloadLength += 2u;

  // Fix the packet size.
  this->size += 2u;

  // Remove padding if present.
  if (this->payloadPadding != 0u) {
    SetPayloadPaddingFlag(false);

    this->size -= size_t{this->payloadPadding};
    this->payloadPadding = 0u;
  }
}

bool RtpPacket::RtxDecode(uint8_t payloadType, uint32_t ssrc) {
  // Chrome sends some RTX packets with no payload when the stream is started.
  // Just ignore them.
  if (this->payloadLength < 2u) return false;

  // Rewrite the payload type.
  SetPayloadType(payloadType);

  // Rewrite the sequence number.
  SetSequenceNumber(Byte::get_2_bytes(this->payload, 0));

  // Rewrite the SSRC.
  SetSsrc(ssrc);

  // Shift the payload to its original place.
  std::memmove(this->payload, this->payload + 2, this->payloadLength - 2);

  // Fix the payload length.
  this->payloadLength -= 2u;

  // Fix the packet size.
  this->size -= 2u;

  // Remove padding if present.
  if (this->payloadPadding != 0u) {
    SetPayloadPaddingFlag(false);

    this->size -= size_t{this->payloadPadding};
    this->payloadPadding = 0u;
  }

  return true;
}

bool RtpPacket::ProcessPayload(codecs::EncodingContext* context, bool& marker) {
  if (!this->payloadDescriptorHandler) return true;

  if (this->payloadDescriptorHandler->Process(context, this->payload, marker)) {
    return true;
  } else {
    return false;
  }
}

void RtpPacket::RestorePayload() {
  if (!this->payloadDescriptorHandler) return;

  this->payloadDescriptorHandler->Restore(this->payload);
}

void RtpPacket::ShiftPayload(size_t payloadOffset, size_t shift, bool expand) {
  if (shift == 0u) return;

  uint8_t* payloadOffsetPtr = this->payload + payloadOffset;
  size_t shiftedLen;

  if (expand) {
    shiftedLen =
        this->payloadLength + size_t{this->payloadPadding} - payloadOffset;

    std::memmove(payloadOffsetPtr + shift, payloadOffsetPtr, shiftedLen);

    this->payloadLength += shift;
    this->size += shift;
  } else {
    shiftedLen = this->payloadLength + size_t{this->payloadPadding} -
                 payloadOffset - shift;

    std::memmove(payloadOffsetPtr, payloadOffsetPtr + shift, shiftedLen);

    this->payloadLength -= shift;
    this->size -= shift;
  }
}

void RtpPacket::ParseExtensions() {}
}  // namespace bifrost
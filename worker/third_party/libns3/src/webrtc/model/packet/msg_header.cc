#include "msg_header.h"

NS_OBJECT_ENSURE_REGISTERED(MsgHeader);

MsgHeader::MsgHeader(PacketType type, uint16_t size)
  :m_type(type),
  m_size(size) {
}

MsgHeader::MsgHeader() {}

MsgHeader::~MsgHeader() {}

TypeId MsgHeader::GetTypeId() {
  static TypeId tid = TypeId("MsgHeader")
    .SetParent<Header>()
    .AddConstructor<MsgHeader>()
  ;
  return tid;  
}

TypeId MsgHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t MsgHeader::GetSerializedSize() const {
  return sizeof(m_type) + sizeof(m_size);
}

void MsgHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU8(static_cast<uint8_t>(m_type));
  start.WriteU16(m_size);
}

uint32_t MsgHeader::Deserialize(Buffer::Iterator start){
  m_type = static_cast<PacketType>(start.ReadU8());
  m_size = start.ReadU16();
  return GetSerializedSize();
}

void MsgHeader::Print (std::ostream& os) const
{    
}
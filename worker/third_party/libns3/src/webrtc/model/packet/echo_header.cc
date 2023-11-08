#include "echo_header.h"

NS_OBJECT_ENSURE_REGISTERED(EchoHeader);

EchoHeader::EchoHeader(uint16_t seq, uint64_t ts)
  :echo_seq_(seq),
  timestamp_(ts) {
}

EchoHeader::EchoHeader() {}

EchoHeader::~EchoHeader() {}

TypeId EchoHeader::GetTypeId() {
  static TypeId tid = TypeId("EchoHeader")
    .SetParent<Header>()
    .AddConstructor<EchoHeader>()
  ;
  return tid;  
}

TypeId EchoHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t EchoHeader::GetSerializedSize() const {
  return sizeof(echo_seq_) + sizeof(timestamp_);
}

void EchoHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU16(echo_seq_);
  start.WriteU16(timestamp_);
}

uint32_t EchoHeader::Deserialize(Buffer::Iterator start){
  echo_seq_ = start.ReadU16();
  timestamp_ = start.ReadU64();

  return GetSerializedSize();
}

void EchoHeader::Print (std::ostream& os) const
{    
}
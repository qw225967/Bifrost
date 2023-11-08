#include "nack_header.h"
NackHeader::NackHeader(uint16_t seq, std::vector<uint16_t> packets)
  :m_nackSeq(seq),
  m_lossPackets(packets)
{}
NackHeader::NackHeader() {}
NackHeader::~NackHeader() {}

TypeId NackHeader::GetTypeId() {
  static TypeId tid = TypeId("NackHeader")
    .SetParent<Header>()
    .AddConstructor<NackHeader>()
  ;
  return tid;
}

TypeId NackHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t NackHeader::GetSerializedSize() const {
  return sizeof(m_nackSeq) + 
          m_lossPackets.size() * (sizeof(uint16_t));
}

void NackHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU8(m_nackSeq);
  start.WriteU16(m_lossPackets.size());
  for(auto seq : m_lossPackets)
  {
    start.WriteU16(seq);
  }
}

uint32_t NackHeader::Deserialize(Buffer::Iterator start) {
  m_nackSeq = start.ReadU16();
  auto size = start.ReadU16();
  for(int i = 0; i < size; i++){
    m_lossPackets.push_back(start.ReadU16());
  }
  return GetSerializedSize();
}

void NackHeader::Print (std::ostream& os) const
{    
}
  
  
  
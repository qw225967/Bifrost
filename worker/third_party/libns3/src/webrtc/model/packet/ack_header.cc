#include "ack_header.h"
#include "ns3/simulator.h"

AckHeader::AckHeader(uint16_t seq, std::map<uint16_t, uint64_t> packets)
  :m_ackSeq(seq),
  m_receivedPackets(packets)
{}
AckHeader::AckHeader(){}
AckHeader::~AckHeader() {}

TypeId AckHeader::GetTypeId() {
  static TypeId tid = TypeId("AckHeader")
    .SetParent<Header>()
    .AddConstructor<AckHeader>()
  ;
  return tid;
}

TypeId AckHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t AckHeader::GetSerializedSize() const {
  return sizeof(m_ackSeq) + 2 + 
          m_receivedPackets.size() * (sizeof(uint16_t) +
          sizeof(uint64_t));
}

void AckHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU16(m_ackSeq);
  start.WriteU16(m_receivedPackets.size());
  for(auto it : m_receivedPackets )
  {
    start.WriteU16(it.first);
    start.WriteU64(it.second);
  }
}

uint32_t AckHeader::Deserialize(Buffer::Iterator start) {
  m_ackSeq = start.ReadU16();
  auto size = start.ReadU16();
  for(int i = 0; i < size; i++){
    m_receivedPackets[start.ReadU16()] = start.ReadU64();
  }
  return GetSerializedSize();
}

void AckHeader::Clear() {
  m_receivedPackets.clear();
}

void AckHeader::Print (std::ostream& os) const
{    
}

void AckHeader::OnRecoveryPacket(uint16_t frame_id, uint16_t seq){
  OnVideoPacket(seq, Simulator::Now().GetMilliSeconds());
}
  
  
  
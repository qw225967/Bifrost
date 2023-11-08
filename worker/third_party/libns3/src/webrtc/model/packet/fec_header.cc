#include "fec_header.h"

NS_OBJECT_ENSURE_REGISTERED(FecHeader);

FecHeader::FecHeader(uint16_t frame_id, uint16_t fec_id, std::vector<uint16_t> packets)
  :m_frameId(frame_id),
  m_fecId(fec_id),
  m_packets(packets) {
}

FecHeader::FecHeader(){}

FecHeader::~FecHeader() {}

TypeId FecHeader::GetTypeId() {
  static TypeId tid = TypeId("FecHeader")
    .SetParent<Header>()
    .AddConstructor<FecHeader>()
  ;
  return tid;  
}

TypeId FecHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t FecHeader::GetSerializedSize() const {
  return sizeof(m_frameId) + sizeof(m_fecId) + sizeof(m_packetNumber) + m_packets.size() * sizeof(uint16_t);
}

void FecHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU16(m_frameId);
  start.WriteU16(m_fecId);
  start.WriteU16(m_packets.size());
  for(auto packet : m_packets){
    start.WriteU16(packet);
  }
}

uint32_t FecHeader::Deserialize(Buffer::Iterator start){
  m_frameId = start.ReadU16();
  m_fecId = start.ReadU16();
  m_packetNumber = start.ReadU16();
  for(int i = 0; i < m_packetNumber; i++){
    m_packets.push_back(start.ReadU16());
  }
  return GetSerializedSize();
}

void FecHeader::Print (std::ostream& os) const
{    
}
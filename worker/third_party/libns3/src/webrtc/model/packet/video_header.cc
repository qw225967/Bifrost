#include "video_header.h"

NS_OBJECT_ENSURE_REGISTERED(VideoHeader);

VideoHeader::VideoHeader(uint16_t seq, uint16_t frameId, 
uint16_t firstSeq, uint16_t packetNumber, uint16_t payloadSize)
  :m_seq(seq),
  m_frameId(frameId),
  m_firstSeq(firstSeq),
  m_packetNumber(packetNumber),
  m_payloadSize(payloadSize) {
}

VideoHeader::VideoHeader() {}

VideoHeader::~VideoHeader() {}

TypeId VideoHeader::GetTypeId() {
  static TypeId tid = TypeId("VideoHeader")
    .SetParent<Header>()
    .AddConstructor<VideoHeader>()
  ;
  return tid;  
}

TypeId VideoHeader::GetInstanceTypeId() const {
  return GetTypeId();
}

uint32_t VideoHeader::GetSerializedSize() const {
  return sizeof(m_seq) + 
         sizeof(m_frameId) + 
         sizeof(m_firstSeq) + 
         sizeof(m_packetNumber) + 
         sizeof(m_payloadSize);
}

void VideoHeader::Serialize(Buffer::Iterator start) const {
  start.WriteU16(m_seq);
  start.WriteU16(m_frameId);
  start.WriteU16(m_firstSeq);
  start.WriteU16(m_packetNumber);
  start.WriteU16(m_payloadSize);
}

uint32_t VideoHeader::Deserialize(Buffer::Iterator start){
  m_seq = start.ReadU16();
  m_frameId = start.ReadU16();
  m_firstSeq = start.ReadU16();
  m_packetNumber = start.ReadU16();
  m_payloadSize = start.ReadU16();  
  return GetSerializedSize();
}

void VideoHeader::Print (std::ostream& os) const
{    
}
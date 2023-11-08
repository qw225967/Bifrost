#ifndef VIDEO_PACKET_H
#define VIDEO_PACKET_H

#include "ns3/header.h"
#include "ns3/type-id.h"

using namespace ns3;
class VideoHeader : public Header {
  public:
    VideoHeader(uint16_t seq, uint16_t frameId, uint16_t firstSeq, uint16_t packetNumber, uint16_t payloadSize);
    VideoHeader();
    ~VideoHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const; 
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    uint16_t Seq() const { return m_seq;}
    uint16_t FrameId() const { return m_frameId;}
    uint16_t FirstSeq() const { return m_firstSeq;}
    uint16_t PacketNumber() const { return m_packetNumber;}
    uint16_t PayloadSize() const { return m_payloadSize;}

    bool IsFirstPacket() const { return m_seq == m_firstSeq;}
    bool IsLastPacket() const { return m_seq == m_firstSeq + m_packetNumber - 1;}
  private:
    uint16_t m_seq;
    uint16_t m_frameId;
    uint16_t m_firstSeq;
    uint16_t m_packetNumber;
    uint16_t m_payloadSize;
};
#endif
#ifndef ACK_HEADER_H
#define ACK_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"

using namespace ns3;
class AckHeader : public Header {
  public:
    AckHeader(uint16_t seq, std::map<uint16_t, uint64_t> packets);
    AckHeader();
    ~AckHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const; 
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;
    void Clear();

    void OnVideoPacket(uint16_t seq, uint64_t now_ms){
      m_receivedPackets[seq] = now_ms;
    }

    void OnRecoveryPacket(uint16_t frame_id, uint16_t seq);

    std::map<uint16_t, uint64_t>& Packets() {
      return m_receivedPackets;
    }
    uint16_t Seq() { return m_ackSeq; }
    void IncreaseAckSeq() { m_ackSeq++;}
  private:
    uint16_t m_ackSeq;
    std::map<uint16_t, uint64_t> m_receivedPackets;
};
#endif
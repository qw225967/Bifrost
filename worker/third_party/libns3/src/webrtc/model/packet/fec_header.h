#ifndef FEC_HEADER_H
#define FEC_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"
#include "ns3/common.h"

using namespace ns3;
class FecHeader : public Header {
  public:
    FecHeader(uint16_t frame_id, uint16_t fec_id, std::vector<uint16_t> packets);
    FecHeader();
    ~FecHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    void SetFrameId(uint16_t frame_id) { m_frameId = frame_id;}
    void SetFecId(uint16_t fec_id) { m_fecId = fec_id; }
    void SetProtectedPackets(std::vector<uint16_t> packets) { m_packets = packets;}

    uint16_t FrameId() const { return m_frameId;}
    uint16_t FecId() const { return m_fecId;}
    uint16_t PacketNumber() const { return m_packetNumber;}
    std::vector<uint16_t> ProtectedPacketSeqs() const { return m_packets;}

  private:
    uint16_t m_frameId;
    uint16_t m_fecId;
    uint16_t m_packetNumber;
    std::vector<uint16_t> m_packets;    
};
#endif
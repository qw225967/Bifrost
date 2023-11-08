#ifndef NACK_HEADER_H
#define NACK_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"

using namespace ns3;
class NackHeader : public Header {
  public:
    NackHeader(uint16_t seq, std::vector<uint16_t> packets);
    NackHeader();
    ~NackHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const; 
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;
  private:
    uint16_t m_nackSeq;
    std::vector<uint16_t> m_lossPackets;
};
#endif
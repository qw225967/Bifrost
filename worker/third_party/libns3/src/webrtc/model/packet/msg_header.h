#ifndef MSG_HEADER_H
#define MSG_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"
#include "ns3/common.h"

using namespace ns3;
class MsgHeader : public Header {
  public:
    MsgHeader(PacketType type, uint16_t size);
    MsgHeader();
    ~MsgHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    PacketType Type() const {return m_type;}
    uint16_t Size() const { return m_size;}

    void SetType(PacketType type) { m_type = type; }
    void SetSize(uint16_t size) {m_size = size;}

  private:
    PacketType m_type; //uint8_t
    uint16_t m_size; //msg siz, not include msg header(3 bytes long)
};



#endif
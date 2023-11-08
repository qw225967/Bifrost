#ifndef ECHO_HEADER_H
#define ECHO_HEADER_H

#include "ns3/header.h"
#include "ns3/type-id.h"
#include "ns3/common.h"

using namespace ns3;
class EchoHeader : public Header {
  public:
    EchoHeader(uint16_t seq, uint64_t ts);
    EchoHeader();
    ~EchoHeader();

    static ns3::TypeId GetTypeId();
    virtual ns3::TypeId GetInstanceTypeId() const;
    virtual uint32_t GetSerializedSize() const;
    virtual void Serialize(ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize(ns3::Buffer::Iterator start);
    virtual void Print (std::ostream& os) const;

    void SetTimestamp(uint64_t ts) { timestamp_ = ts;}
    void IncreaseEchoSeq() { echo_seq_++;}

    uint16_t Seq() const {return echo_seq_;}
    uint64_t Ts() const { return timestamp_;}

  private:
    uint16_t echo_seq_; //uint8_t
    uint64_t timestamp_; //msg siz, not include msg header(3 bytes long)
};
#endif
/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/16 10:34 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_tcc.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_TCC_H
#define WORKER_RTCP_TCC_H

#include <vector>

#include "common.h"
#include "rtcp_feedback.h"

/* RTP Extensions for Transport-wide Congestion Control
 * draft-holmer-rmcat-transport-wide-cc-extensions-01

   0               1               2               3
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |V=2|P|  FMT=15 |    PT=205     |           length              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     SSRC of packet sender                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      SSRC of media source                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |      base sequence number     |      packet status count      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 reference time                | fb pkt. count |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          packet chunk         |         packet chunk          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                                                               .
  .                                                               .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         packet chunk          |  recv delta   |  recv delta   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                                                               .
  .                                                               .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           recv delta          |  recv delta   | zero padding  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

namespace bifrost {
class FeedbackRtpTransportPacket : public FeedbackRtpPacket {
 public:
  struct PacketResult {
    PacketResult(uint16_t sequenceNumber, bool received)
        : sequenceNumber(sequenceNumber), received(received) {}

    uint16_t sequenceNumber;  // Wide sequence number.
    int16_t delta{0};         // Delta.
    bool received{false};     // Packet received or not.
    int64_t receivedAtMs{
        0};  // Received time (ms) in remote timestamp reference.
  };

 public:
  enum class AddPacketResult { SUCCESS = 0, MAX_SIZE_EXCEEDED = 1, FATAL };

 private:
  enum Status : uint8_t {
    NotReceived = 0,
    SmallDelta,
    LargeDelta,
    Reserved,
    None
  };

 private:
  struct Context {
    bool allSameStatus{true};
    Status currentStatus{Status::None};
    std::vector<Status> statuses;
  };

 private:
  class Chunk {
   public:
    static Chunk* Parse(const uint8_t* data, size_t len, uint16_t count);

   public:
    Chunk() = default;
    virtual ~Chunk() = default;

    virtual bool AddDeltas(const uint8_t* data, size_t len,
                           std::vector<int16_t>& deltas, size_t& offset) = 0;
    virtual void Dump() const = 0;
    virtual uint16_t GetCount() const = 0;
    virtual uint16_t GetReceivedStatusCount() const = 0;
    virtual void FillResults(std::vector<struct PacketResult>& packetResults,
                             uint16_t& currentSequenceNumber) const = 0;
    virtual size_t Serialize(uint8_t* buffer) = 0;
  };

 private:
  class RunLengthChunk : public Chunk {
   public:
    RunLengthChunk(Status status, uint16_t count)
        : status(status), count(count) {}
    explicit RunLengthChunk(uint16_t buffer);

   public:
    bool AddDeltas(const uint8_t* data, size_t len,
                   std::vector<int16_t>& deltas, size_t& offset) override;
    Status GetStatus() const { return this->status; }
    void Dump() const override;
    uint16_t GetCount() const override { return this->count; }
    uint16_t GetReceivedStatusCount() const override;
    void FillResults(std::vector<struct PacketResult>& packetResults,
                     uint16_t& currentSequenceNumber) const override;
    size_t Serialize(uint8_t* buffer) override;

   private:
    Status status{Status::None};
    uint16_t count{0u};
  };

 private:
  class OneBitVectorChunk : public Chunk {
   public:
    explicit OneBitVectorChunk(const std::vector<Status>& statuses)
        : statuses(statuses) {}
    OneBitVectorChunk(uint16_t buffer, uint16_t count);

   public:
    bool AddDeltas(const uint8_t* data, size_t len,
                   std::vector<int16_t>& deltas, size_t& offset) override;
    void Dump() const override;
    uint16_t GetCount() const override { return this->statuses.size(); }
    uint16_t GetReceivedStatusCount() const override;
    void FillResults(std::vector<struct PacketResult>& packetResults,
                     uint16_t& currentSequenceNumber) const override;
    size_t Serialize(uint8_t* buffer) override;

   private:
    std::vector<Status> statuses;
  };

 private:
  class TwoBitVectorChunk : public Chunk {
   public:
    explicit TwoBitVectorChunk(const std::vector<Status>& statuses)
        : statuses(statuses) {}
    TwoBitVectorChunk(uint16_t buffer, uint16_t count);

   public:
    bool AddDeltas(const uint8_t* data, size_t len,
                   std::vector<int16_t>& deltas, size_t& offset) override;
    void Dump() const override;
    uint16_t GetCount() const override { return this->statuses.size(); }
    uint16_t GetReceivedStatusCount() const override;
    void FillResults(std::vector<struct PacketResult>& packetResults,
                     uint16_t& currentSequenceNumber) const override;
    size_t Serialize(uint8_t* buffer) override;

   private:
    std::vector<Status> statuses;
  };

 public:
  static size_t fixedHeaderSize;
  static uint16_t maxMissingPackets;
  static uint16_t maxPacketStatusCount;
  static int16_t maxPacketDelta;

 public:
  static std::shared_ptr<FeedbackRtpTransportPacket> Parse(const uint8_t* data,
                                                           size_t len);

 private:
  static std::map<Status, std::string> status2String;

 public:
  FeedbackRtpTransportPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
      : FeedbackRtpPacket(FeedbackRtp::MessageType::TCC, senderSsrc,
                          mediaSsrc) {}
  FeedbackRtpTransportPacket(CommonHeader* commonHeader, size_t availableLen);
  ~FeedbackRtpTransportPacket();

 public:
  AddPacketResult AddPacket(uint16_t sequenceNumber, uint64_t timestamp,
                            size_t maxRtcpPacketLen);
  void Finish();  // Just for locally generated packets.
  bool IsFull() {
    // NOTE: Since AddPendingChunks() is called at the end, we cannot track
    // the exact ongoing value of packetStatusCount. Hence, let's reserve 7
    // packets just in case.
    return this->packetStatusCount >=
           FeedbackRtpTransportPacket::maxPacketStatusCount - 7;
  }
  bool IsSerializable() const { return this->deltas.size() > 0; }
  bool IsCorrect() const  // Just for locally generated packets.
  {
    return this->isCorrect;
  }
  uint16_t GetBaseSequenceNumber() const { return this->baseSequenceNumber; }
  uint16_t GetPacketStatusCount() const { return this->packetStatusCount; }
  int32_t GetReferenceTime() const { return this->referenceTime; }
  int64_t GetReferenceTimestamp() const  // Reference time in ms.
  {
    return static_cast<int64_t>(this->referenceTime) * 64;
  }
  uint8_t GetFeedbackPacketCount() const { return this->feedbackPacketCount; }
  void SetFeedbackPacketCount(uint8_t count) {
    this->feedbackPacketCount = count;
  }
  uint16_t GetLatestSequenceNumber()
      const  // Just for locally generated packets.
  {
    return this->latestSequenceNumber;
  }
  uint64_t GetLatestTimestamp() const  // Just for locally generated packets.
  {
    return this->latestTimestamp;
  }
  std::vector<struct PacketResult> GetPacketResults() const;
  uint8_t GetPacketFractionLost() const;

  /* Pure virtual methods inherited from Packet. */
 public:
  void Dump() const override;
  size_t Serialize(uint8_t* buffer) override;
  size_t GetSize() const override {
    if (this->size) return this->size;

    // Fixed packet size.
    size_t size = FeedbackRtpPacket::GetSize();

    size += FeedbackRtpTransportPacket::fixedHeaderSize;
    size += this->deltasAndChunksSize;

    // 32 bits padding.
    size += (-size) & 3;

    return size;
  }

 private:
  void FillChunk(uint16_t previousSequenceNumber, uint16_t sequenceNumber,
                 int16_t delta);
  void CreateRunLengthChunk(Status status, uint16_t count);
  void CreateOneBitVectorChunk(std::vector<Status>& statuses);
  void CreateTwoBitVectorChunk(std::vector<Status>& statuses);
  void AddPendingChunks();

 private:
  uint16_t baseSequenceNumber{0u};
  int32_t referenceTime{0};
  uint16_t latestSequenceNumber{0u};  // Just for locally generated packets.
  uint64_t latestTimestamp{0u};       // Just for locally generated packets.
  uint16_t packetStatusCount{0u};
  uint8_t feedbackPacketCount{0u};
  std::vector<Chunk*> chunks;
  std::vector<int16_t> deltas;
  Context context;  // Just for locally generated packets.
  size_t deltasAndChunksSize{0u};
  size_t size{0};
  bool isCorrect{true};
};
}  // namespace bifrost

#endif  // WORKER_RTCP_TCC_H

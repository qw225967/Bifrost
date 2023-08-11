/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 10:25 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : nack.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_NACK_H
#define WORKER_NACK_H

#include <unordered_map>
#include <vector>

#include "common.h"
#include "quiche/quic/core/quic_types.h"
#include "quiche/quic/core/quic_unacked_packet_map.h"
#include "rtcp_nack.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
class Player;
class Nack : UvTimer::Listener {
 public:
  static constexpr uint16_t MaxValue = std::numeric_limits<uint16_t>::max();
  struct NackInfo {
    NackInfo() = default;
    explicit NackInfo(uint16_t seq, uint16_t sendAtSeq, uint64_t ms,
                      uint8_t time)
        : seq(seq), sent_time_ms(ms), retries(time) {}

    uint16_t seq{0u};
    uint64_t sent_time_ms{0u};
    uint8_t retries{0u};
  };

  enum class NackFilter { SEQ, TIME };

  struct StorageItem {
    // Cloned packet.
    RtpPacketPtr packet;
    // Last time this packet was resent.
    uint64_t sent_time_ms{0u};
    // Number of times this packet was resent.
    uint8_t sent_times{0u};
  };

 public:
  Nack(uint32_t ssrc, UvLoop** uv_loop);
  Nack(uint32_t ssrc, UvLoop** uv_loop, Player* player);
  ~Nack();

  // OnTimer
  void OnTimer(UvTimer* timer);

 public:
  // send
  void OnSendRtpPacket(RtpPacketPtr& rtp_packet);
  void ReceiveNack(FeedbackRtpNackPacket* packet,
                   std::vector<RtpPacketPtr>& vec);

  // recv
  void OnReceiveRtpPacket(RtpPacketPtr rtp_packet);
  void OnNackGeneratorNack(const std::vector<uint16_t>& seqNumbers);

  bool SeqLowerThan(const uint16_t lhs, const uint16_t rhs) {
    return ((rhs > lhs) && (rhs - lhs <= MaxValue / 2)) ||
           ((lhs > rhs) && (lhs - rhs > MaxValue / 2));
  }
  bool SeqHigherThan(const uint16_t lhs, const uint16_t rhs) {
    return ((lhs > rhs) && (lhs - rhs <= MaxValue / 2)) ||
           ((rhs > lhs) && (rhs - lhs > MaxValue / 2));
  }

  size_t GetNackListLength() const { return this->recv_nack_list_.size(); }
  void UpdateRtt(uint32_t rtt) { this->rtt_ = rtt; }
  std::vector<uint16_t> GetNackBatch();

 private:
  // send
  void FillRetransmissionContainer(uint16_t seq, uint16_t bitmask,
                                   std::vector<RtpPacketPtr>& vec);

  // recv
  void AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd);

 private:
  // base
  UvLoop* uv_loop_;
  Player* player_;
  UvTimer* nack_timer_;

  // send
  uint32_t ssrc_;
  std::unordered_map<uint16_t, StorageItem> send_rtp_packet_map_;
  bool send_init_ = false;
  uint64_t max_send_ms_;

  // recv
  std::map<uint16_t, NackInfo> recv_nack_list_;
  bool recv_started_{false};
  uint16_t last_seq_{0u};  // Seq number of last valid packet.
  uint32_t rtt_{0u};       // Round trip time (ms).
};
}  // namespace bifrost

#endif  // WORKER_NACK_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/30 10:25 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : nack.cpp
 * @description : TODO
 *******************************************************/

#include "nack.h"

#include "player.h"
#include "rtcp_nack.h"

namespace bifrost {
constexpr uint32_t MaxRetransmissionDelay{3000};  // 原 2000
constexpr size_t MaxPacketAge{10000u};
constexpr size_t MaxNackPackets{1000u};
constexpr uint32_t DefaultRtt{10u};
constexpr uint8_t MaxNackRetries{20u};
constexpr uint64_t TimerInterval{20u};

/* Instance methods. */
Nack::Nack(uint32_t ssrc, UvLoop** uv_loop)
    : ssrc_(ssrc), rtt_(DefaultRtt), uv_loop_(*uv_loop) {}

Nack::Nack(uint32_t ssrc, UvLoop** uv_loop, Player* player)
    : ssrc_(ssrc),
      rtt_(DefaultRtt),
      uv_loop_(*uv_loop),
      player_(std::move(player)) {
  this->nack_timer_ = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->nack_timer_->Start(TimerInterval, TimerInterval);
}

Nack::~Nack() {
  this->send_rtp_packet_map_.clear();
  this->recv_nack_list_.clear();
  delete this->nack_timer_;
}

void Nack::OnSendRtpPacket(RtpPacketPtr rtp_packet, uint16_t ext_seq) {
  auto seq = rtp_packet->GetSequenceNumber();

  if (!send_init_) {
    send_init_ = true;
  }

  auto ts = this->uv_loop_->get_time_ms_int64();
  StorageItem item;
  item.packet = rtp_packet;
  item.sent_times = 0;
  item.sent_time_ms = ts;
  item.extension_seq = ext_seq;

  this->max_send_ms_ = (max_send_ms_ > ts ? max_send_ms_ : ts);

  this->send_rtp_packet_map_[seq] = item;
}

std::vector<RtpPacketPtr> Nack::ReceiveNack(
    FeedbackRtpNackPacket* packet, quic::LostPacketVector& lost_packet,
    quic::QuicByteCount& bytes_in_flight) {
  std::vector<RtpPacketPtr> result;
  for (auto it = packet->Begin(); it != packet->End(); ++it) {
    FeedbackRtpNackItem* item = *it;
    FillRetransmissionContainer(item->GetPacketId(),
                                item->GetLostPacketBitmask(), result,
                                lost_packet, bytes_in_flight);
  }
  return result;
}

void Nack::FillRetransmissionContainer(uint16_t seq, uint16_t bitmask,
                                       std::vector<RtpPacketPtr>& vec,
                                       quic::LostPacketVector& lost_packet,
                                       quic::QuicByteCount& bytes_in_flight) {
  // Look for each requested packet.
  uint64_t now_ms = this->uv_loop_->get_time_ms();
  uint16_t rtt = (this->rtt_ != 0u ? this->rtt_ : DefaultRtt);
  uint16_t current_seq = seq;
  bool requested{true};

  while (requested || bitmask != 0) {
    if (requested) {
      auto iter = this->send_rtp_packet_map_.find(current_seq);
      if (iter != this->send_rtp_packet_map_.end()) {
        uint32_t diff_ms;

        auto packet = iter->second.packet;
        bytes_in_flight -= iter->second.packet->GetSize();

        diff_ms = this->max_send_ms_ - iter->second.sent_time_ms;

        if (diff_ms > MaxRetransmissionDelay) {
          lost_packet.push_back(quic::LostPacket(quic::LostPacket(
              quic::QuicPacketNumber(iter->second.extension_seq),
              iter->second.packet->GetSize())));
          this->send_rtp_packet_map_.erase(iter);
        } else if (now_ms - iter->second.sent_time_ms <=
                   static_cast<uint64_t>(rtt / 4)) {
          // do nothing
        } else {
          // Save when this packet was resent.
          iter->second.sent_time_ms = now_ms;

          // Increase the number of times this packet was sent.
          iter->second.sent_times++;

          vec.push_back(packet);
        }
      }
    }

    requested = (bitmask & 1) != 0;
    bitmask >>= 1;
    ++current_seq;
  }
}

void Nack::OnNackGeneratorNack(const std::vector<uint16_t>& seqNumbers) {
  std::shared_ptr<FeedbackRtpNackPacket> packet =
      std::make_shared<FeedbackRtpNackPacket>(0, ssrc_);

  auto it = seqNumbers.begin();
  const auto end = seqNumbers.end();
  size_t numPacketsRequested{0};

  while (it != end) {
    uint16_t seq;
    uint16_t bitmask{0};

    seq = *it;
    ++it;

    while (it != end) {
      uint16_t shift = *it - seq - 1;

      if (shift > 15) break;

      bitmask |= (1 << shift);
      ++it;
    }

    auto* nackItem = new FeedbackRtpNackItem(seq, bitmask);

    packet->AddItem(nackItem);

    numPacketsRequested += nackItem->CountRequestedPackets();
  }

  // Ensure that the RTCP packet fits into the RTCP buffer.
  if (packet->GetSize() > BufferSize) {
    return;
  }

  packet->Serialize(Buffer);

  this->player_->OnSendRtcpNack(packet);
}

void Nack::OnReceiveRtpPacket(RtpPacketPtr rtp_packet) {
  uint16_t seq = rtp_packet->GetSequenceNumber();

  if (!this->recv_started_) {
    this->recv_started_ = true;
    this->last_seq_ = seq;

    return;
  }

  // Obviously never nacked, so ignore.
  if (seq == this->last_seq_) return;

  // May be an out of order packet, or already handled retransmitted packet,
  // or a retransmitted packet.
  if (this->SeqLowerThan(seq, this->last_seq_)) {
    auto it = this->recv_nack_list_.find(seq);

    // It was a nacked packet.
    if (it != this->recv_nack_list_.end()) {
      this->recv_nack_list_.erase(it);

      return;
    }

    return;
  }

  this->AddPacketsToNackList(this->last_seq_ + 1, seq);
  this->last_seq_ = seq;
}

void Nack::AddPacketsToNackList(uint16_t seq_start, uint16_t seq_end) {
  // Remove old packets.
  auto it = this->recv_nack_list_.lower_bound(seq_end - MaxPacketAge);

  this->recv_nack_list_.erase(this->recv_nack_list_.begin(), it);

  // If the nack list is too large, remove packets from the nack list until
  // the latest first packet of a keyframe. If the list is still too large,
  // clear it and request a keyframe.
  uint16_t num_new_nacks = seq_end - seq_start;

  if (this->recv_nack_list_.size() + num_new_nacks > MaxNackPackets) {
    if (this->recv_nack_list_.size() + num_new_nacks > MaxNackPackets) {
      this->recv_nack_list_.clear();
      return;
    }
  }

  for (uint16_t seq = seq_start; seq != seq_end; ++seq) {
    if (this->recv_nack_list_.find(seq) == this->recv_nack_list_.end())
      this->recv_nack_list_.emplace(
          std::make_pair(seq, NackInfo{seq, seq, 0, 0}));
  }
}

double CalculateRttLimit2SendNack(int tryTimes) {
  return tryTimes < 3 ? (double)(tryTimes * 0.4) + 1 : 2;
}

std::vector<uint16_t> Nack::GetNackBatch() {
  uint64_t now_ms = uv_loop_->get_time_ms_int64();
  NackFilter filter = NackFilter::TIME;
  std::vector<uint16_t> nack_batch;

  auto it = this->recv_nack_list_.begin();

  while (it != this->recv_nack_list_.end()) {
    NackInfo& nack_info = it->second;
    uint16_t seq = nack_info.seq;

    auto limit_var =
        uint64_t(this->rtt_ / CalculateRttLimit2SendNack(nack_info.retries));

    if (filter == NackFilter::TIME &&
        now_ms - nack_info.sent_time_ms >= limit_var) {
      nack_batch.emplace_back(seq);
      nack_info.retries++;
      auto old_ms = nack_info.sent_time_ms;
      if (old_ms == 0) {
        old_ms = now_ms;
      }

      nack_info.sent_time_ms = now_ms;
      std::cout << "retry seq:" << seq
                << ", times:" << unsigned(nack_info.retries)
                << ", interval:" << now_ms - old_ms << std::endl;
      if (nack_info.retries >= MaxNackRetries) {
        it = this->recv_nack_list_.erase(it);
      } else {
        ++it;
      }

      continue;
    }

    ++it;
  }
  return nack_batch;
}

void Nack::OnTimer(UvTimer* timer) {
  if (this->nack_timer_ == timer) {
    auto nack_batch = this->GetNackBatch();
    if (nack_batch.empty()) return;
    this->OnNackGeneratorNack(nack_batch);
  }
}

}  // namespace bifrost
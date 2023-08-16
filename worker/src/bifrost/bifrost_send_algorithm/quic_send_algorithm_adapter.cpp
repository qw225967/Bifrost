/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 4:13 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : quic_send_algorithm_adapter.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost_send_algorithm/quic_send_algorithm_adapter.h"

#include "quiche/quic/core/congestion_control/rtt_stats.h"

namespace bifrost {

QuicSendAlgorithmAdapter::QuicSendAlgorithmAdapter(UvLoop** uv_loop) {
  uv_loop_ = *uv_loop;

  this->clock_ = new QuicClockAdapter(uv_loop);
  this->rtt_stats_ = new quic::RttStats();
  this->rtt_stats_->set_initial_rtt(
      quic::QuicTimeDelta::FromMilliseconds(rtt_));
  this->unacked_packet_map_ =
      new quic::QuicUnackedPacketMap(quic::Perspective::IS_CLIENT);
  this->random_ = quiche::QuicheRandom::GetInstance();
  this->connection_stats_ = new quic::QuicConnectionStats();
  this->send_algorithm_interface_ = quic::SendAlgorithmInterface::Create(
      this->clock_, this->rtt_stats_, this->unacked_packet_map_,
      congestion_type, this->random_, this->connection_stats_, 1, nullptr);
}

QuicSendAlgorithmAdapter::~QuicSendAlgorithmAdapter() {
  delete this->clock_;
  delete this->rtt_stats_;
  delete this->unacked_packet_map_;
  delete this->random_;
  delete this->connection_stats_;
  delete this->send_algorithm_interface_;
}

void QuicSendAlgorithmAdapter::OnRtpPacketSend(RtpPacket rtp_packet) {
  quic::QuicTime ts =
      quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(now);

  if (!(*it)->IsReTrans()) {
    // 1 quic 记录发送信息
    SendPacketInfo temp;
    temp.sequence = rtp_packet->GetSequenceNumber();
    temp.send_time = now;
    temp.send_bytes = rtp_packet->GetSize();
    temp.is_retrans = rtp_packet->IsReTrans();
    if (has_send_map_.find(temp.sequence) == has_send_map_.end())
      this->has_send_map_[temp.sequence] = temp;

    // 4.放入unack，此处不统计重复数据，使用无重传模式
    if (this->unacked_packet_map_ != nullptr) {
      this->unacked_packet_map_->AddSentPacket(
          (*it), (*it)->GetSequenceNumber(), this->largest_acked_seq_,
          quic::NOT_RETRANSMISSION, ts, true, true, quic::ECN_NOT_ECT);
    }
    this->bytes_in_flight_ += temp.send_bytes;
  }

  // 1.2 算法记录发送
  if (this->send_algorithm_interface_ != nullptr) {
    this->send_algorithm_interface_->OnPacketSent(
        ts, this->unacked_packet_map_->bytes_in_flight(),
        quic::QuicPacketNumber(rtp_packet->GetSequenceNumber()),
        rtp_packet->GetSize(), quic::HAS_RETRANSMITTABLE_DATA);
  }
}

void QuicSendAlgorithmAdapter::OnReceiveQuicAckFeedback(QuicAckFeedbackPacket* feedback) {
  auto now_ms = this->uv_loop_->get_time_ms_int64();
  int64_t transport_rtt_sum = 0;
  int64_t transport_rtt_count = 0;

  uint32_t ack_bytes = 0;

  for (auto it = feedback->Begin(); it != feedback->End(); ++it) {
    QuicAckFeedbackItem* item = *it;

    quic::QuicTime recv_time =
        quic::QuicTime::Zero() +
        quic::QuicTimeDelta::FromMilliseconds(now_ms - item->GetDelta());

    // 1.确认数据
    acked_packets_.push_back(
        quic::AckedPacket(quic::QuicPacketNumber(item->GetSequence()),
                          item->GetRecvBytes(), recv_time));

    auto ack_it = has_send_map_.find(item->GetSequence());
    if (ack_it != has_send_map_.end()) {
      ack_bytes += ack_it->second.send_bytes;
      bytes_in_flight_ -= ack_it->second.send_bytes;

      // 2.移除确认的数据
      if (this->unacked_packet_map_ != nullptr) {
        this->unacked_packet_map_->RemoveFromInFlight(
            quic::QuicPacketNumber(item->GetSequence()));
      }

      transport_rtt_sum += now_ms - ack_it->second.send_time - item->GetDelta();
      transport_rtt_count++;

      has_send_map_.erase(ack_it);
    }

    largest_acked_seq_ = item->GetSequence();
  }

  // 3.确认当前最新seq
  if (this->unacked_packet_map_ != nullptr) {
    this->unacked_packet_map_->IncreaseLargestAcked(
        quic::QuicPacketNumber(this->largest_acked_seq_));
    this->unacked_packet_map_->RemoveObsoletePackets();
  }

  // 4.更新rtt
  quic::QuicTime quic_now =
      quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(now_ms);
  auto transport_rtt = transport_rtt_sum / transport_rtt_count;
  transport_rtt_ = transport_rtt == 0
      ? transport_rtt
      : (transport_rtt_ * 8 + transport_rtt) / 9;
  if (rtt_stats_ != nullptr) {
    rtt_stats_->UpdateRtt(quic::QuicTimeDelta::FromMilliseconds(5),
                          quic::QuicTimeDelta::FromMilliseconds(transport_rtt),
                          quic_now);
  }

  // 5.进行拥塞事件触发
  if (this->send_algorithm_interface_ != nullptr) {
    // 5.1 删除旧数据,并更新当前周期丢包数据
    this->RemoveOldSendPacket();

    // bbr v1 无需最后两个数据，随意给个值
    this->send_algorithm_interface_->OnCongestionEvent(
        true, this->unacked_packet_map_->bytes_in_flight(), quic_now,
        this->acked_packets_, this->losted_packets_, quic::QuicPacketCount(0),
        quic::QuicPacketCount(0));

    if (!this->send_algorithm_interface_
    ->PacingRate(this->unacked_packet_map_->bytes_in_flight())
    .IsZero()) {
      this->pacer_bits_ =
          this->send_algorithm_interface_
          ->PacingRate(this->unacked_packet_map_->bytes_in_flight())
          .ToBitsPerSecond();
      if (send_algorithm_interface_->GetCongestionWindow() != 0) {
        this->cwnd_ = this->send_algorithm_interface_->GetCongestionWindow();
      } else {
        this->cwnd_ = 6000;
      }
    }
  }

  acked_packets_.clear();
  losted_packets_.clear();
}

void QuicSendAlgorithmAdapter::RemoveOldSendPacket() {
  auto now = this->uv_loop_->get_time_ms_int64();
  int64_t remove_interval = MaxIntervalSendPacketRemove +
                            transport_rtt_ / 2;  // 1个feedback时间+传输的rtt/2

  // 1.把重传时间内的丢失数据统计出来
  auto it = has_send_map_.begin();
  while (it != has_send_map_.end()) {
    // 大于周重传未确认则判定丢失
    if (now - it->second.send_time > remove_interval) {
      this->losted_packets_.push_back(quic::LostPacket(
          quic::QuicPacketNumber(it->second.sequence), it->second.send_bytes));

      feedback_lost_no_count_packet_vec_.push_back(it->second);
      it = has_send_map_.erase(it);
    } else {
      it++;
    }
  }

  // 2.存在上行feedback丢失，把丢失的ack信令中回复的feedback数据去掉
  auto vec_it = feedback_lost_no_count_packet_vec_.begin();
  while (vec_it != feedback_lost_no_count_packet_vec_.end()) {
    if (now - vec_it->send_time > MaxFeedbackAckDoNotTransTime) {
      if (this->unacked_packet_map_ != nullptr) {
        this->unacked_packet_map_->RemoveFromInFlight(
            quic::QuicPacketNumber(vec_it->sequence));
      }
      this->bytes_in_flight_ -= vec_it->send_bytes;
      vec_it = feedback_lost_no_count_packet_vec_.erase(vec_it);
    } else {
      vec_it++;
    }
  }
  if (this->unacked_packet_map_)
    this->unacked_packet_map_->RemoveObsoletePackets();
}
}  // namespace bifrost
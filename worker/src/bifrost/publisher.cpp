/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : publisher.cpp
 * @description : TODO
 *******************************************************/

#include "publisher.h"

#include "quiche/quic/core/congestion_control/rtt_stats.h"
#include "rtcp_compound_packet.h"

namespace bifrost {
const uint32_t InitialAvailableGccBitrate = 1000000u;
const uint32_t InitialAvailableBBRBitrate = 1000000u;
const uint16_t IntervalSendTime = 5u;
const uint32_t IntervalDataDump = 1000u;
const uint32_t IntervalSendReport = 2000u;
const uint32_t MaxIntervalSendPacketRemove = 200u; // 一个feedback时间
const uint32_t MaxFeedbackAckDoNotTransTime = 4000u;

Publisher::Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
                     Observer* observer, uint8_t number,
                     ExperimentManagerPtr& experiment_manager,
                     quic::CongestionControlType congestion_type)
    : remote_addr_config_(remote_config),
      uv_loop_(*uv_loop),
      observer_(observer),
      pacer_bits_(
          (congestion_type == quic::kBBR || congestion_type == quic::kBBRv2)
              ? InitialAvailableBBRBitrate
              : InitialAvailableGccBitrate),
      rtt_(20),
      experiment_manager_(experiment_manager),
      number_(number),
      congestion_type_(congestion_type) {
  std::cout << "publish experiment manager:" << experiment_manager << std::endl;

  // 1.remote address set
  auto remote_addr = Settings::get_sockaddr_by_config(remote_config);
  this->udp_remote_address_ = std::make_shared<sockaddr>(remote_addr);

  // 3.timer start
  this->producer_timer_ = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->producer_timer_->Start(IntervalSendTime, IntervalSendTime);
  this->data_dump_timer_ = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->data_dump_timer_->Start(IntervalDataDump, IntervalDataDump);
  this->send_report_timer_ =
      new UvTimer(this, this->uv_loop_->get_loop().get());
  this->send_report_timer_->Start(IntervalSendReport, IntervalSendReport);

  // 4.nack
  nack_ = std::make_shared<Nack>(remote_addr_config_.ssrc, uv_loop);

  // 5.create data producer
  this->data_producer_ =
      std::make_shared<DataProducer>(remote_addr_config_.ssrc);

  // 6.congestion control
  switch (congestion_type) {
    case quic::kGoogCC:
      this->tcc_client_ = std::make_shared<TransportCongestionControlClient>(
          this, InitialAvailableGccBitrate, &this->uv_loop_);
      break;
    case quic::kCubicBytes:
    case quic::kRenoBytes:
    case quic::kBBR:
    case quic::kPCC:
    case quic::kBBRv2:
      clock_ = new QuicClockAdapter(uv_loop);
      rtt_stats_ = new quic::RttStats();
      rtt_stats_->set_initial_rtt(quic::QuicTimeDelta::FromMilliseconds(rtt_));
      unacked_packet_map_ =
          new quic::QuicUnackedPacketMap(quic::Perspective::IS_CLIENT);
      random_ = quiche::QuicheRandom::GetInstance();
      connection_stats_ = new quic::QuicConnectionStats();
      send_algorithm_interface_ = quic::SendAlgorithmInterface::Create(
          clock_, rtt_stats_, unacked_packet_map_, congestion_type, random_,
          connection_stats_, 0, nullptr);
      break;
    default:
      this->tcc_client_ = std::make_shared<TransportCongestionControlClient>(
          this, InitialAvailableGccBitrate, &this->uv_loop_);
      break;
  }

  // 7. ssrc
  ssrc_ = remote_addr_config_.ssrc;
}

void Publisher::ReceiveSendAlgorithmFeedback(QuicAckFeedbackPacket* feedback) {
  auto now_ms = this->uv_loop_->get_time_ms_int64();
  int64_t transport_rtt_sum = 0;
  int64_t transport_rtt_count = 0;

  uint32_t ack_bytes = 0;

  for (auto it = feedback->Begin(); it != feedback->End(); ++it) {
    QuicAckFeedbackItem* item = *it;

    quic::QuicTime recv_time =
        quic::QuicTime::Zero() +
        quic::QuicTimeDelta::FromMilliseconds(now_ms - item->GetDelta());

    auto ack_it = has_send_map_.find(item->GetSequence());
    if (ack_it != has_send_map_.end()) {

      // 1.确认数据
      acked_packets_.push_back(
          quic::AckedPacket(quic::QuicPacketNumber(ack_it->second.sequence),
                            ack_it->second.send_bytes, recv_time));
      ack_bytes += ack_it->second.send_bytes;
      bytes_in_flight_ -= ack_it->second.send_bytes;

      // 2.移除确认的数据
      if (this->unacked_packet_map_ != nullptr) {
        this->unacked_packet_map_->RemoveFromInFlight(
            quic::QuicPacketNumber(item->GetSequence()));
      }

      transport_rtt_sum += ack_it->second.send_time - now_ms - item->GetDelta();
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
  transport_rtt_ = transport_rtt > 0 ? transport_rtt : transport_rtt_;
  if (rtt_stats_ != nullptr) {
    rtt_stats_->UpdateRtt(quic::QuicTimeDelta::FromMilliseconds(5),
                          quic::QuicTimeDelta::FromMilliseconds(transport_rtt),
                          quic_now);
  }

  // 5.进行拥塞事件触发
  if (this->send_algorithm_interface_ != nullptr) {
    // 5.1 删除旧数据,并更新当前周期丢包数据
    this->RemoveOldSendPacket();

    if (is_app_limit_) {
      this->send_algorithm_interface_->OnApplicationLimited(
          this->unacked_packet_map_->bytes_in_flight());
    } else {
      this->send_algorithm_interface_->LeaveApplicationLimited();
    }

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
    }
  }

  acked_packets_.clear();
  losted_packets_.clear();
}

void Publisher::GetRtpExtensions(RtpPacketPtr& packet) {
  static uint8_t buffer[4096];
  uint8_t extenLen = 2u;
  static std::vector<RtpPacket::GenericExtension> extensions;
  // This happens just once.
  if (extensions.capacity() != 24) extensions.reserve(24);

  extensions.clear();

  uint8_t* bufferPtr{buffer};
  // NOTE: Add value 0. The sending Transport will update it.
  uint16_t wideSeqNumber{0u};

  Byte::set_2_bytes(bufferPtr, 0, wideSeqNumber);
  extensions.emplace_back(static_cast<uint8_t>(7), extenLen, bufferPtr);
  packet->SetExtensions(2, extensions);
  packet->SetTransportWideCc01ExtensionId(7);
}

void Publisher::OnReceiveNack(FeedbackRtpNackPacket* packet) {
  std::vector<RtpPacketPtr> packets;
  this->nack_->ReceiveNack(packet, packets, &this->unacked_packet_map_,
                           this->bytes_in_flight_, this->has_send_map_);

  for (auto& pkt : packets) {
    pkt->SetIsReTrans();
    this->pacer_vec_.push_back(pkt);
  }
}

void Publisher::RemoveOldSendPacket() {
  auto now = this->uv_loop_->get_time_ms_int64();
  int64_t remove_interval = MaxIntervalSendPacketRemove + transport_rtt_/2; // 1个feedback时间+传输的rtt/2

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

void Publisher::TimerSendPacket(int32_t available) {
  available += this->pre_remind_bits_;

  auto now = this->uv_loop_->get_time_ms_int64();
  auto it = pacer_vec_.begin();
  while (it != pacer_vec_.end() && available > 0) {

    quic::QuicTime ts =
        quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(now);

    if (!(*it)->IsReTrans()) {
      // 1 quic 记录发送信息
      SendPacketInfo temp;
      temp.sequence = (*it)->GetSequenceNumber();
      temp.send_time = now;
      temp.send_bytes = (*it)->GetSize();
      temp.is_retrans = (*it)->IsReTrans();
      this->has_send_map_[temp.sequence] = temp;

      // 4.放入unack，此处不统计重复数据，使用无重传模式
      if (this->unacked_packet_map_ != nullptr) {
        this->unacked_packet_map_->AddSentPacket(
            (*it), (*it)->GetSequenceNumber(),
            this->largest_acked_seq_, quic::NOT_RETRANSMISSION, ts, true,
            true, quic::ECN_NOT_ECT);
      }
      this->bytes_in_flight_ += temp.send_bytes;
    }

    // 1.2 算法记录发送
    if (this->send_algorithm_interface_ != nullptr) {
      this->send_algorithm_interface_->OnPacketSent(
          ts, this->unacked_packet_map_->bytes_in_flight(),
          quic::QuicPacketNumber((*it)->GetSequenceNumber()), (*it)->GetSize(),
          quic::HAS_RETRANSMITTABLE_DATA);
    }

    // 2.tcc
    this->GetRtpExtensions((*it));
    (*it)->UpdateTransportWideCc01(this->tcc_seq_++);

    if (this->tcc_client_ != nullptr) {
      webrtc::RtpPacketSendInfo packetInfo;

      packetInfo.ssrc = (*it)->GetSsrc();
      packetInfo.transport_sequence_number = this->tcc_seq_;
      packetInfo.has_rtp_sequence_number = true;
      packetInfo.rtp_sequence_number = (*it)->GetSequenceNumber();
      packetInfo.length = (*it)->GetSize();
      packetInfo.pacing_info = this->tcc_client_->GetPacingInfo();

      // webrtc中发送和进入发送状态有一小段等待时间
      // 因此分开了两个函数 insert 和 sent 函数
      this->tcc_client_->InsertPacket(packetInfo);

      this->tcc_client_->PacketSent(packetInfo, now);
    }

    observer_->OnPublisherSendPacket((*it), this->udp_remote_address_.get());

    this->send_bits_prior_ += (*it)->GetSize() * 8;
    this->send_packet_bytes_ += (*it)->GetSize();
    available -= int32_t((*it)->GetSize() * 8);
    this->send_packet_count_++;

    it = pacer_vec_.erase(it);
  }

  this->pre_remind_bits_ = available;
}

void Publisher::OnReceiveReceiverReport(ReceiverReport* report) {
  // Get the NTP representation of the current timestamp.
  uint64_t nowMs = this->uv_loop_->get_time_ms();
  auto ntp = Time::TimeMs2Ntp(nowMs);

  // Get the compact NTP representation of the current timestamp.
  uint32_t compactNtp = (ntp.seconds & 0x0000FFFF) << 16;

  compactNtp |= (ntp.fractions & 0xFFFF0000) >> 16;

  uint32_t lastSr = report->GetLastSenderReport();
  uint32_t dlsr = report->GetDelaySinceLastSenderReport();

  // RTT in 1/2^16 second fractions.
  uint32_t rtt{0};

  // If no Sender Report was received by the remote endpoint yet, ignore lastSr
  // and dlsr values in the Receiver Report.
  if (lastSr && (compactNtp > dlsr + lastSr)) rtt = compactNtp - dlsr - lastSr;

  // RTT in milliseconds.
  this->rtt_ = static_cast<float>(rtt >> 16) * 1000;
  this->rtt_ += (static_cast<float>(rtt & 0x0000FFFF) / 65536) * 1000;

  webrtc::RTCPReportBlock webrtc_report;
  webrtc_report.last_sender_report_timestamp = report->GetLastSenderReport();
  webrtc_report.source_ssrc = report->GetSsrc();
  webrtc_report.jitter = report->GetDelaySinceLastSenderReport();
  webrtc_report.fraction_lost = report->GetFractionLost();
  webrtc_report.packets_lost = report->GetTotalLost();
  //
  //  std::cout << "receive rr "
  //            << "last_sender_report_timestamp:" <<
  //            report->GetLastSenderReport()
  //            << ", ssrc:" << report->GetSsrc()
  //            << ", jitter:" << report->GetDelaySinceLastSenderReport()
  //            << ", fraction_lost:" << uint32_t(report->GetFractionLost())
  //            << ", packets_lost:" << report->GetTotalLost()
  //            << ", rtt:" << this->rtt_ << std::endl;

  this->nack_->UpdateRtt(uint32_t(this->rtt_));

  quic::QuicTime now = quic::QuicTime::Zero() +
                       quic::QuicTimeDelta::FromMilliseconds(
                           (int64_t)this->uv_loop_->get_time_ms_int64());
  if (this->rtt_stats_) {
    this->rtt_stats_->UpdateRtt(
        quic::QuicTimeDelta::FromMilliseconds(0),
        quic::QuicTimeDelta::FromMilliseconds((int64_t)this->rtt_), now);
  }

  if (this->tcc_client_ != nullptr) {
    this->tcc_client_->ReceiveRtcpReceiverReport(
        webrtc_report, this->rtt_, this->uv_loop_->get_time_ms_int64());
  }
}

SenderReport* Publisher::GetRtcpSenderReport(uint64_t nowMs) {
  auto ntp = Time::TimeMs2Ntp(nowMs);
  auto* report = new SenderReport();

  // Calculate TS difference between now and maxPacketMs.
  auto diffMs = nowMs - this->max_packet_ms_;
  auto diffTs =
      diffMs * 90000 / 1000;  // 现实中常用的采样换算此处写死 90000 - 视频

  report->SetSsrc(this->ssrc_);
  report->SetPacketCount(this->send_packet_count_);
  report->SetOctetCount(this->send_packet_bytes_);
  report->SetNtpSec(ntp.seconds);
  report->SetNtpFrac(ntp.fractions);
  report->SetRtpTs(this->max_packet_ts_ + diffTs);

  return report;
}

void Publisher::OnTimer(UvTimer* timer) {
  if (timer == this->producer_timer_) {
    int32_t available = int32_t(this->pacer_bits_ * 5 * 1.2 / 1000);
    int32_t limit = 1200000 * 5 * 1.2 / 1000;

    if (available > limit) {
      is_app_limit_ = true;
    } else {
      is_app_limit_ = false;
    }
    available = available > (limit) ? (limit) : available;
    auto send_available = available;

    while (available > 0) {
      if (this->data_producer_ != nullptr) {
        auto packet = this->data_producer_->CreateData(available);
        if (packet == nullptr) {
          break;
        }
        auto now = this->uv_loop_->get_time_ms_int64();
        this->max_packet_ms_ = now;
        this->max_packet_ts_ = now;

        // 1.webrtc 内部会把空间删除，在这里做个拷贝
        auto len = packet->capacity() + packet->size();
        auto* payload_data = new uint8_t[len];
        memcpy(payload_data, packet->data(), len);
        RtpPacketPtr rtp_packet = RtpPacket::Parse(payload_data, len);
        rtp_packet->SetPayloadDataPtr(&payload_data);
        // 2.nack模块
        this->OnSendPacketInNack(rtp_packet);

        pacer_vec_.push_back(rtp_packet);

        available -= int32_t((packet->capacity() + packet->size()) * 8);

        delete packet;
      }
    }

    this->TimerSendPacket(send_available);
  }

  if (timer == this->data_dump_timer_) {
    if (this->tcc_client_ != nullptr) {
      auto gcc_available = this->tcc_client_->get_available_bitrate();
      std::vector<double> trends = this->tcc_client_->get_trend();

      for (auto i = 0; i < trends.size(); i++) {
        ExperimentGccData gcc_data_temp(0, 0, trends[i]);
        this->experiment_manager_->DumpGccDataToCsv(
            this->number_, i + 1, trends.size(), gcc_data_temp);
      }

      ExperimentGccData gcc_data(gcc_available, this->send_bits_prior_, 0);
      this->send_bits_prior_ = 0;
      this->experiment_manager_->DumpGccDataToCsv(this->number_, 1, 1,
                                                  gcc_data);
    }
    if (this->send_algorithm_interface_ != nullptr) {
      std::cout << "show bytes_in_flight_:" << bytes_in_flight_ << std::endl;
      this->send_algorithm_interface_->DebugShow();

      auto available = this->send_algorithm_interface_->BandwidthEstimate()
                           .ToBitsPerSecond();
      std::vector<double> trends = {};

      for (auto i = 0; i < trends.size(); i++) {
        ExperimentGccData gcc_data_temp(0, 0, trends[i]);
        this->experiment_manager_->DumpGccDataToCsv(
            this->number_, i + 1, trends.size(), gcc_data_temp);
      }

      ExperimentGccData data(available, this->send_bits_prior_, 0);
      this->send_bits_prior_ = 0;
      this->experiment_manager_->DumpGccDataToCsv(this->number_, 1, 1, data);
    }
  }

  if (timer == this->send_report_timer_) {
    // 立刻回复sr
    uint64_t now = this->uv_loop_->get_time_ms_int64();
    std::shared_ptr<CompoundPacket> packet = std::make_shared<CompoundPacket>();
    auto* report = GetRtcpSenderReport(now);
    packet->AddSenderReport(report);
    packet->Serialize(Buffer);
    this->observer_->OnPublisherSendRtcpPacket(packet,
                                               this->udp_remote_address_.get());
  }
}
}  // namespace bifrost
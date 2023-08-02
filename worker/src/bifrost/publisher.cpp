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
const uint32_t InitialAvailableBitrate = 1000000u;
const uint16_t IntervalSendTime = 5u;
const uint32_t IntervalDataDump = 1000u;
const uint32_t IntervalSendReport = 2000u;

Publisher::Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
                     Observer* observer, uint8_t number,
                     ExperimentManagerPtr& experiment_manager,
                     quic::CongestionControlType congestion_type)
    : remote_addr_config_(remote_config),
      uv_loop_(*uv_loop),
      observer_(observer),
      pacer_bits_(InitialAvailableBitrate),
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
          this, InitialAvailableBitrate, &this->uv_loop_);
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
          this, InitialAvailableBitrate, &this->uv_loop_);
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
  auto it = feedback->Begin();
  uint16_t pre_seq = 0;
  if (it != feedback->End())
    pre_seq = (*it)->GetSequence();

  for (; it != feedback->End(); ++it) {
    QuicAckFeedbackItem* item = *it;
    quic::QuicTime recv_time =
        quic::QuicTime::Zero() +
        quic::QuicTimeDelta::FromMilliseconds(now_ms - item->GetDelta());

    acked_packets_.push_back(
        quic::AckedPacket(quic::QuicPacketNumber(item->GetSequence()),
                          item->GetRecvBytes(), recv_time));
    ack_bytes += item->GetRecvBytes();
    bytes_in_flight_ -= item->GetRecvBytes();

    if (item->GetSequence() > pre_seq + 1) {
      for (int i= pre_seq + 1; i < item->GetSequence(); i++) {
        auto map_it = has_send_map_.find(item->GetSequence());
        if (map_it != has_send_map_.end()) {
          losted_packets_.push_back(
              quic::LostPacket(quic::QuicPacketNumber(map_it->second.sequence),
                               map_it->second.send_bytes));
          has_send_map_.erase(map_it);
        }
      }
    }

    auto map_it = has_send_map_.find(item->GetSequence());
    if (map_it != has_send_map_.end()) {
      transport_rtt_sum += map_it->second.send_time - now_ms - item->GetDelta();
      transport_rtt_count++;
      has_send_map_.erase(map_it);
    }
    if (this->unacked_packet_map_ != nullptr) {
      this->unacked_packet_map_->RemoveFromInFlight(
          quic::QuicPacketNumber(item->GetSequence()));
      this->unacked_packet_map_->RemoveObsoletePackets();
    }
    largest_acked_seq_ = item->GetSequence();
  }

  if (this->unacked_packet_map_ != nullptr) {
    this->unacked_packet_map_->IncreaseLargestAcked(
        quic::QuicPacketNumber(this->largest_acked_seq_));
  }

  quic::QuicTime quic_now =
      quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(now_ms);
  auto transport_rtt = transport_rtt_sum / transport_rtt_count;
  transport_rtt_ = transport_rtt > 0 ? transport_rtt : transport_rtt_;
  if (rtt_stats_ != nullptr) {
    rtt_stats_->UpdateRtt(quic::QuicTimeDelta::FromMilliseconds(5),
                          quic::QuicTimeDelta::FromMilliseconds(transport_rtt),
                          quic_now);
  }
  if (this->send_algorithm_interface_ != nullptr) {
    // bbr v1 无需最后两个数据，随意给个值
    this->send_algorithm_interface_->OnCongestionEvent(
        true, this->bytes_in_flight_, quic_now, this->acked_packets_,
        this->losted_packets_, quic::QuicPacketCount(0),
        quic::QuicPacketCount(0));

    if (!this->send_algorithm_interface_->PacingRate(this->bytes_in_flight_)
             .IsZero()) {
      this->pacer_bits_ =
          this->send_algorithm_interface_->PacingRate(this->bytes_in_flight_)
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
  this->nack_->ReceiveNack(packet, packets);
  quic::QuicTime ts =
      quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(
                                   this->uv_loop_->get_time_ms_int64());
  for (auto pkt : packets) {
    if (this->unacked_packet_map_ != nullptr) {
      this->unacked_packet_map_->AddSentPacket(
          pkt, pkt->GetSequenceNumber(), this->largest_acked_seq_,
          quic::LOSS_RETRANSMISSION, ts, true, false, quic::ECN_NOT_ECT);
    }

    this->GetRtpExtensions(pkt);
    pkt->UpdateTransportWideCc01(this->tcc_seq_++);

    this->observer_->OnPublisherSendPacket(pkt,
                                           this->udp_remote_address_.get());
    this->bytes_in_flight_ += pkt->GetSize();
    if (this->send_algorithm_interface_ != nullptr) {
      this->send_algorithm_interface_->OnPacketSent(
          ts, bytes_in_flight_, quic::QuicPacketNumber(pkt->GetSequenceNumber()),
          packet->GetSize(), quic::HAS_RETRANSMITTABLE_DATA);
    }
  }
}

uint32_t Publisher::TccClientSendRtpPacket(std::shared_ptr<uint8_t> data,
                                           size_t len) {
  RtpPacketPtr packet = RtpPacket::Parse(data, len);

  // 1.quic 记录发送信息
  SendPacketInfo temp;
  temp.sequence = packet->GetSequenceNumber();
  temp.send_time = this->uv_loop_->get_time_ms_int64();
  temp.send_bytes = packet->GetSize();

  this->has_send_map_[temp.sequence] = temp;
  bytes_in_flight_ += packet->GetSize();

  quic::QuicTime ts =
      quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(
                                   this->uv_loop_->get_time_ms_int64());
  if (this->unacked_packet_map_ != nullptr) {
    this->unacked_packet_map_->AddSentPacket(
        packet, temp.sequence, this->largest_acked_seq_,
        quic::NOT_RETRANSMISSION, ts, true, true, quic::ECN_NOT_ECT);
  }

  if (this->send_algorithm_interface_ != nullptr) {
    this->send_algorithm_interface_->OnPacketSent(
        ts, bytes_in_flight_, quic::QuicPacketNumber(temp.sequence),
        packet->GetSize(), quic::NO_RETRANSMITTABLE_DATA);
  }

  // 2.nack模块
  this->OnSendPacketInNack(packet);

  // 3.tcc
  this->GetRtpExtensions(packet);
  packet->UpdateTransportWideCc01(this->tcc_seq_++);

  if (this->tcc_client_ != nullptr) {
    webrtc::RtpPacketSendInfo packetInfo;

    packetInfo.ssrc = packet->GetSsrc();
    packetInfo.transport_sequence_number = this->tcc_seq_;
    packetInfo.has_rtp_sequence_number = true;
    packetInfo.rtp_sequence_number = packet->GetSequenceNumber();
    packetInfo.length = packet->GetSize();
    packetInfo.pacing_info = this->tcc_client_->GetPacingInfo();

    // webrtc中发送和进入发送状态有一小段等待时间
    // 因此分开了两个函数 insert 和 sent 函数
    this->tcc_client_->InsertPacket(packetInfo);

    this->tcc_client_->PacketSent(packetInfo,
                                  this->uv_loop_->get_time_ms_int64());
  }

  observer_->OnPublisherSendPacket(packet, this->udp_remote_address_.get());
  return packet->GetSize();
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
    int32_t available =
        int32_t(this->pacer_bits_ * 1.25 / 200) +
        pre_remind_bits_;  // 5ms 一次发送定时，但每次多发25%数据
    if (congestion_type_ == quic::kBBR) {
      available *= 1.25;
    }

    available = available > (1200000 / 200) ? (1200000 / 200) : available;
    while (available > 0) {
      if (this->data_producer_ != nullptr) {
        auto packet = this->data_producer_->CreateData(available);
        if (packet == nullptr) {
          break;
        }
        this->max_packet_ms_ = this->uv_loop_->get_time_ms_int64();
        this->max_packet_ts_ = this->uv_loop_->get_time_ms_int64();
        std::shared_ptr<uint8_t> temp_data(
            new uint8_t[packet->capacity() + packet->size()]);
        memcpy(temp_data.get(), packet->data(),
               packet->capacity() + packet->size());
        auto send_size = this->TccClientSendRtpPacket(
            temp_data, packet->capacity() + packet->size());

        available -= int32_t(send_size * 8);
        this->send_bits_prior_ += (send_size * 8);
        this->send_packet_bytes_ += send_size;
        this->send_packet_count_++;

        delete packet;
      }
    }
    pre_remind_bits_ = available;
  }

  if (timer == this->data_dump_timer_) {
    if (this->send_algorithm_interface_) {
      std::cout << "show bytes_in_flight_:" << bytes_in_flight_ << std::endl;
      this->send_algorithm_interface_->DebugShow();
    }

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
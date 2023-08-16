/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : publisher.cpp
 * @description : TODO
 *******************************************************/

#include "publisher.h"


#include "rtcp_compound_packet.h"

namespace bifrost {
const uint32_t InitialAvailableGccBitrate = 1000000u;
const uint32_t InitialAvailableBBRBitrate = 1000000u;
const uint16_t IntervalSendTime = 5u;
const uint32_t IntervalDataDump = 1000u;
const uint32_t IntervalSendReport = 2000u;
const uint32_t MaxIntervalSendPacketRemove = 200u;  // 一个feedback时间
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

      break;
    default:
      this->tcc_client_ = std::make_shared<TransportCongestionControlClient>(
          this, InitialAvailableGccBitrate, &this->uv_loop_);
      break;
  }

  // 7. ssrc
  ssrc_ = remote_addr_config_.ssrc;
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

  for (auto& pkt : packets) {
    pkt->SetIsReTrans();
    this->pacer_vec_.push_back(pkt);
  }
}

uint32_t Publisher::TimerSendPacket(int32_t available_bytes) {
  uint32_t used_bytes = 0;
  available_bytes += this->pre_remind_bytes_;
  auto now = this->uv_loop_->get_time_ms_int64();
  auto it = pacer_vec_.begin();
  while (it != pacer_vec_.end() && available_bytes > 0) {
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
    available_bytes -= int32_t((*it)->GetSize());
    used_bytes += (*it)->GetSize();
    this->send_packet_count_++;

    it = pacer_vec_.erase(it);
  }
  this->pre_remind_bytes_ = available_bytes;
  return used_bytes;
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

    available = available > (limit) ? (limit) : available;
    auto send_available = available / 8;

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
    if (this->send_algorithm_interface_ != nullptr) {
      if (this->cwnd_ > 0) {
        uint32_t used_bytes = this->TimerSendPacket(send_available);
        this->cwnd_ -= (int32_t)used_bytes;
      }
    } else {
      this->TimerSendPacket(send_available);
    }
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
      //      std::cout << "show bytes_in_flight_:" << bytes_in_flight_ <<
      //      std::endl;
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
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
const uint32_t InitialAvailableBitrate = 600000u;
const uint16_t IntervalSendTime = 5u;
const uint32_t IntervalDataDump = 1000u;
const uint32_t IntervalSendReport = 2000u;

Publisher::Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
                     Observer* observer)
    : remote_addr_config_(remote_config),
      uv_loop_(*uv_loop),
      observer_(observer),
      pacer_bits_(InitialAvailableBitrate),
      rtt_(100) {
  // 1.remote address set
  auto remote_addr = Settings::get_sockaddr_by_config(remote_config);
  this->udp_remote_address_ = std::make_shared<sockaddr>(remote_addr);

  // 2.experiment
  this->experiment_manager_ = std::make_shared<ExperimentManager>();

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

  // 6.tcc client
  this->tcc_client_ = std::make_shared<TransportCongestionControlClient>(
      this, InitialAvailableBitrate, &this->uv_loop_);

  // 7. ssrc
  ssrc_ = remote_addr_config_.ssrc;
}

void Publisher::GetRtpExtensions(RtpPacketPtr packet) {
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

uint32_t Publisher::TccClientSendRtpPacket(const uint8_t* data, size_t len) {
  RtpPacketPtr packet = RtpPacket::Parse(data, len);

  this->OnSendPacketInNack(packet);

  this->GetRtpExtensions(packet);
  packet->UpdateTransportWideCc01(++this->tcc_seq_);

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

  observer_->OnPublisherSendPacket(packet, this->udp_remote_address_.get());
  return packet->GetSize();
}

void Publisher::OnReceiveReceiverReport(ReceiverReport* report) {
  webrtc::RTCPReportBlock webrtc_report;
  webrtc_report.last_sender_report_timestamp = report->GetLastSenderReport();
  webrtc_report.source_ssrc = report->GetSsrc();
  webrtc_report.jitter = report->GetDelaySinceLastSenderReport();
  webrtc_report.fraction_lost = report->GetFractionLost();
  webrtc_report.packets_lost = report->GetTotalLost();
  this->tcc_client_->ReceiveRtcpReceiverReport(
      webrtc_report, rtt_, this->uv_loop_->get_time_ms_int64());
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

    available = available > (1200000 / 200) ? (1200000 / 200) : available;
    while (available > 0) {
      if (this->data_producer_ != nullptr) {
        auto packet = this->data_producer_->CreateData(available);
        if (packet == nullptr) {
          break;
        }
        this->max_packet_ms_ = this->uv_loop_->get_time_ms_int64();
        this->max_packet_ts_ = this->uv_loop_->get_time_ms_int64();
        auto send_size = this->TccClientSendRtpPacket(
            packet->data(), packet->capacity() + packet->size());
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
    auto gcc_available = this->tcc_client_->get_available_bitrate();
    std::vector<double> trends = this->tcc_client_->get_trend();

    for (auto i = 0; i < trends.size(); i++) {
      ExperimentGccData gcc_data_temp(0, 0, trends[i]);
      this->experiment_manager_->DumpGccDataToCsv(i + 1, trends.size(),
                                                  gcc_data_temp);
    }

    ExperimentGccData gcc_data(gcc_available, this->send_bits_prior_, 0);
    this->send_bits_prior_ = 0;
    this->experiment_manager_->DumpGccDataToCsv(1, 1, gcc_data);
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
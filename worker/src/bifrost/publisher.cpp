/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : publisher.cpp
 * @description : TODO
 *******************************************************/

#include "publisher.h"

namespace bifrost {
const uint32_t InitialAvailableBitrate = 600000u;

Publisher::Publisher(Settings::Configuration& remote_config, UvLoop** uv_loop,
                     Observer* observer)
    : remote_addr_config_(remote_config),
      uv_loop_(*uv_loop),
      observer_(observer),
      pacer_bits_(InitialAvailableBitrate) {
  // 1.remote address set
  auto remote_addr = Settings::get_sockaddr_by_config(remote_config);
  this->udp_remote_address_ = std::make_shared<sockaddr>(remote_addr);

  // 2.experiment
  this->experiment_manager_ = std::make_shared<ExperimentManager>();

  // 3.timer start
  this->producer_timer = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->producer_timer->Start(5, 5);
  this->data_dump_timer = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->data_dump_timer->Start(1000, 1000);

  // 4.create data producer
  this->data_producer_ =
      std::make_shared<DataProducer>(remote_addr_config_.ssrc);

  // 5.tcc client
  this->tcc_client_ = std::make_shared<TransportCongestionControlClient>(
      this, InitialAvailableBitrate, &this->uv_loop_);
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

void Publisher::OnTimer(UvTimer* timer) {
  if (timer == this->producer_timer) {
    int32_t available = int32_t(this->pacer_bits_ / 200) + pre_remind_bits_;

    //    available = available > (1200000 / 200) ? (1200000 / 200) : available;
    while (available > 0) {
      if (this->data_producer_ != nullptr) {
        auto packet = this->data_producer_->CreateData();
        if (packet == nullptr) {
          return;
        }
        auto send_size = this->TccClientSendRtpPacket(
            packet->data(), packet->capacity() + packet->size());
        available -= int32_t(send_size * 8);
        this->send_bits_prior_ += (send_size * 8);
        delete packet;
      }
    }
    pre_remind_bits_ = available;
  }

  if (timer == this->data_dump_timer) {
    auto gcc_available = this->tcc_client_->get_available_bitrate();
    std::vector<double> trends = this->tcc_client_->get_trend();

    for (auto i = 0; i<trends.size(); i++) {
      ExperimentGccData gcc_data_temp(0, 0, trends[i]);
      this->experiment_manager_->DumpGccDataToCsv(i+1, trends.size(), gcc_data_temp);
    }

    ExperimentGccData gcc_data(gcc_available, this->send_bits_prior_, 0);
    this->send_bits_prior_ = 0;
    this->experiment_manager_->DumpGccDataToCsv(1, 1, gcc_data);
  }
}
}  // namespace bifrost
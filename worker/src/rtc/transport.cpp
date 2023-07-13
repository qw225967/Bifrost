/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:09 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : transport.cpp
 * @description : TODO
 *******************************************************/

#include "transport.h"

#include "rtcp_compound_packet.h"
#include "rtcp_nack.h"

namespace bifrost {
Transport::Transport(TransportModel model, uint8_t number,
                     ExperimentManagerPtr& experiment_manager)
    : model_(model), number_(number), experiment_manager_(experiment_manager) {
  this->uv_loop_ = new UvLoop;

  // 1.init loop
  this->uv_loop_->ClassInit();

  auto remote_config = Settings::config_.remote_send_configuration_;
  remote_config.ssrc += number;

  auto local_config = Settings::config_.local_receive_configuration_;
  local_config.rtcPort += number;

  // 2.publish and player
  switch (model_) {
    case SinglePublish: {
      // 2.publisher
      this->publisher_ = std::make_shared<Publisher>(
          remote_config, &this->uv_loop_, this, number, experiment_manager);
      break;
    }
  }

    // 4.get router
    //   4.1 don't use default model : just get config default
#ifdef USING_DEFAULT_AF_CONFIG
  this->udp_router_ =
      std::make_shared<UdpRouter>(this->uv_loop_->get_loop().get(), this);
#else
  //   4.2 use default model : get config by json file
  this->udp_router_ = std::make_shared<UdpRouter>(
      local_config, this->uv_loop_->get_loop().get(), this);
#endif
}

Transport::~Transport() {
  if (this->uv_loop_ != nullptr) delete this->uv_loop_;
  if (this->udp_router_ != nullptr) this->udp_router_.reset();
  if (this->publisher_ != nullptr) this->publisher_.reset();

  auto iter = this->players_.begin();
  for (; iter != this->players_.end(); iter++) {
    iter->second.reset();
  }
}

void Transport::Run() { this->uv_loop_->RunLoop(); }

void Transport::OnUdpRouterRtpPacketReceived(
    bifrost::UdpRouter* socket, RtpPacketPtr rtp_packet,
    const struct sockaddr* remote_addr) {
  uint16_t wideSeqNumber;
  rtp_packet->ReadTransportWideCc01(wideSeqNumber);

  //  std::cout << "ssrc:" << rtp_packet->GetSsrc()
  //            << ", seq:" << rtp_packet->GetSequenceNumber()
  //            << ", payload_type:" << rtp_packet->GetPayloadType()
  //            << ", tcc seq:" << wideSeqNumber << ", this:" << this <<
  //            std::endl;
  if (model_ == SinglePublish) return;
  auto player_iter = this->players_.find(rtp_packet->GetSsrc());
  if (player_iter != this->players_.end()) {
    player_iter->second->OnReceiveRtpPacket(rtp_packet);
  } else {
    auto player = std::make_shared<Player>(remote_addr, &this->uv_loop_, this,
                                           rtp_packet->GetSsrc(), this->number_,
                                           this->experiment_manager_);
    this->players_[rtp_packet->GetSsrc()] = player;
  }
}

void Transport::OnUdpRouterRtcpPacketReceived(
    bifrost::UdpRouter* socket, RtcpPacketPtr rtcp_packet,
    const struct sockaddr* remote_addr) {
  auto type = rtcp_packet->GetType();
  switch (type) {
    case Type::RR: {
      auto* rr = static_cast<ReceiverReportPacket*>(rtcp_packet.get());
      for (auto it = rr->Begin(); it != rr->End(); ++it) {
        auto& report = *it;
        this->publisher_->OnReceiveReceiverReport(report);
      }
      break;
    }
    case Type::SR: {
      auto* sr = dynamic_cast<SenderReportPacket*>(rtcp_packet.get());
      for (auto it = sr->Begin(); it != sr->End(); ++it) {
        auto& report = *it;
        auto player_iter = this->players_.find(report->GetSsrc());
        if (player_iter != this->players_.end()) {
          player_iter->second->OnReceiveSenderReport(report);

          // 立刻回复rr
          std::unique_ptr<CompoundPacket> packet =
              std::make_unique<CompoundPacket>();
          auto* report = player_iter->second->GetRtcpReceiverReport();
          packet->AddReceiverReport(report);
          packet->Serialize(Buffer);
          this->udp_router_->Send(packet->GetData(), packet->GetSize(),
                                  remote_addr, nullptr);
        }
      }
      break;
    }
    case Type::RTPFB: {
      auto* rtp_fb = static_cast<FeedbackRtpPacket*>(rtcp_packet.get());
      switch (rtp_fb->GetMessageType()) {
        case FeedbackRtp::MessageType::TCC: {
          auto* feedback = static_cast<FeedbackRtpTransportPacket*>(rtp_fb);
          this->publisher_->ReceiveFeedbackTransport(feedback);
          break;
        }
        case FeedbackRtp::MessageType::NACK: {
          auto* nackPacket = static_cast<FeedbackRtpNackPacket*>(rtp_fb);
          this->publisher_->OnReceiveNack(nackPacket);
          break;
        }
      }
      break;
    }
  }
}
}  // namespace bifrost
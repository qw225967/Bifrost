/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:09 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : transport.cpp
 * @description : TODO
 *******************************************************/

#include "transport.h"

namespace bifrost {
Transport::Transport(TransportModel model) : model_(model) {
  this->uv_loop_ = new UvLoop;

  // 1.init loop
  this->uv_loop_->ClassInit();

  // 2.publish and player
  switch (model_) {
    case SinglePublish: {
      // 2.publisher
      this->publisher_ = std::make_shared<Publisher>(
          Settings::publisher_config_.remote_send_configuration_,
          &this->uv_loop_, this);
      break;
    }
    case SinglePlay: {
      // 2.player
      for (auto config : Settings::player_config_map_) {
        auto player = std::make_shared<Player>(
            config.second.remote_send_configuration_, &this->uv_loop_, this);
        this->players_[config.second.local_receive_configuration_.ssrc] =
            player;
        break;
      }
      break;
    }
    case SinglePublishAndPlays: {
      // 2.publisher
      this->publisher_ = std::make_shared<Publisher>(
          Settings::publisher_config_.remote_send_configuration_,
          &this->uv_loop_, this);

      // 3.player
      for (auto config : Settings::player_config_map_) {
        auto player = std::make_shared<Player>(
            config.second.remote_send_configuration_, &this->uv_loop_, this);
        this->players_[config.second.local_receive_configuration_.ssrc] =
            player;
      }
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
  switch (model_) {
    case SinglePublish: {
      this->udp_router_ = std::make_shared<UdpRouter>(
          Settings::publisher_config_.local_receive_configuration_,
          this->uv_loop_->get_loop().get(), this);
      break;
    }
    case SinglePlay: {
      if (Settings::player_config_map_.empty()) {
        return;
      }
      this->udp_router_ =
          std::make_shared<UdpRouter>(Settings::player_config_map_.begin()
                                          ->second.local_receive_configuration_,
                                      this->uv_loop_->get_loop().get(), this);
      break;
    }
    case SinglePublishAndPlays: {
      this->udp_router_ = std::make_shared<UdpRouter>(
          Settings::publisher_config_.local_receive_configuration_,
          this->uv_loop_->get_loop().get(), this);
      break;
    }
  }
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

  auto player = this->players_.find(rtp_packet->GetSsrc());
  if (player != this->players_.end()) {
    player->second->IncomingPacket(rtp_packet);
  }
}

void Transport::OnUdpRouterRtcpPacketReceived(
    bifrost::UdpRouter* socket, RtcpPacketPtr rtcp_packet,
    const struct sockaddr* remote_addr) {
  auto type = rtcp_packet->GetType();
  switch (type) {
    case Type::RTPFB: {
      auto* rtp_fb = static_cast<FeedbackRtpPacket*>(rtcp_packet.get());
      switch (rtp_fb->GetMessageType()) {
        case FeedbackRtp::MessageType::TCC: {
          auto* feedback = static_cast<FeedbackRtpTransportPacket*>(rtp_fb);
          this->publisher_->ReceiveFeedbackTransport(feedback);
          break;
        }
      }
      break;
    }
  }
}
}  // namespace bifrost
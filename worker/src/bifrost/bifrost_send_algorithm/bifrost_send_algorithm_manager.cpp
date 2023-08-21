/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 10:49 上午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_send_algorithm.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost_send_algorithm/bifrost_send_algorithm_manager.h"
#include "bifrost_send_algorithm/quic_send_algorithm_adapter.h"
#include "bifrost_send_algorithm/tcc_client.h"

#include "rtcp_feedback.h"

namespace bifrost {
const uint32_t InitialAvailableGccBitrate = 1000000u;
const uint32_t InitialAvailableBBRBitrate = 1000000u;

BifrostSendAlgorithmManager::BifrostSendAlgorithmManager(
    quic::CongestionControlType congestion_algorithm_type, UvLoop** uv_loop) {
  switch (congestion_algorithm_type) {
    case quic::kCubicBytes:
    case quic::kRenoBytes:
    case quic::kBBR:
    case quic::kPCC:
    case quic::kBBRv2:
      this->algorithm_interface_= std::make_shared<QuicSendAlgorithmAdapter>(uv_loop);
      break;
    case quic::kGoogCC:
      this->algorithm_interface_ = std::make_shared<TransportCongestionControlClient>(
          this, InitialAvailableGccBitrate, uv_loop);
      break;
  }
}

inline void BifrostSendAlgorithmManager::OnRtpPacketSend(RtpPacket rtp_packet, int64_t now) {
  this->algorithm_interface_->OnRtpPacketSend(rtp_packet, now);
}

inline void BifrostSendAlgorithmManager::OnReceiveRtcpFeedback(const FeedbackRtpPacket* fb) {
  this->algorithm_interface_->OnReceiveRtcpFeedback(rtcp_packet);
}

inline void BifrostSendAlgorithmManager::OnReceiveReceiverReport(ReceiverReport* report) {
  this->algorithm_interface_->OnReceiveReceiverReport(report);
}

inline void BifrostSendAlgorithmManager::UpdateRtt(float rtt) {
  this->algorithm_interface_->UpdateRtt(rtt);
}
}  // namespace bifrost
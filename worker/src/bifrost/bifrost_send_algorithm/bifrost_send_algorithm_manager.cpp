/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 10:49 上午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_send_algorithm.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost_send_algorithm/bifrost_send_algorithm_manager.h"
#include "rtcp_feedback.h"

namespace bifrost {
const uint32_t InitialAvailableGccBitrate = 600000u;

BifrostSendAlgorithmManager::BifrostSendAlgorithmManager(
    quic::CongestionControlType congestion_algorithm_type, UvLoop** uv_loop) {
  switch (congestion_algorithm_type) {
    case quic::kCubicBytes:
    case quic::kRenoBytes:
    case quic::kBBR:
    case quic::kPCC:
    case quic::kBBRv2:
      this->algorithm_interface_ = std::make_shared<QuicSendAlgorithmAdapter>(uv_loop, congestion_algorithm_type);
      break;
    case quic::kGoogCC:
      this->algorithm_interface_ = std::make_shared<TransportCongestionControlClient>(
          this, InitialAvailableGccBitrate, uv_loop);
      break;
  }
}

void BifrostSendAlgorithmManager::OnRtpPacketSend(RtpPacketPtr rtp_packet, int64_t nowMs) {
  this->algorithm_interface_->OnRtpPacketSend(rtp_packet, nowMs);
}

void BifrostSendAlgorithmManager::OnReceiveRtcpFeedback(FeedbackRtpPacket* fb) {
  this->algorithm_interface_->OnReceiveRtcpFeedback(fb);
}

void BifrostSendAlgorithmManager::OnReceiveReceiverReport(webrtc::RTCPReportBlock report,
                                                                 float rtt, int64_t nowMs) {
  this->algorithm_interface_->OnReceiveReceiverReport(report, rtt, nowMs);
}

void BifrostSendAlgorithmManager::UpdateRtt(float rtt) {
  this->algorithm_interface_->UpdateRtt(rtt);
}
}  // namespace bifrost
/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 4:12 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : QuicSendAlgorithmAdapter.h
 * @description : TODO
 *******************************************************/

#ifndef _QUICSENDALGORITHMADAPTER_H
#define _QUICSENDALGORITHMADAPTER_H

#include "bifrost_send_algorithm_interface.h"
#include "bifrost/bifrost_send_algorithm/quic_clock_adapter.h"
#include "quiche/quic/core/congestion_control/send_algorithm_interface.h"

#include "rtcp_quic_feedback.h"

namespace bifrost{
class QuicSendAlgorithmAdapter : public BifrostSendAlgorithmInterface {
 public:
  QuicSendAlgorithmAdapter(UvLoop** uv_loop);
  ~QuicSendAlgorithmAdapter();

 public:
  // BifrostSendAlgorithmInterface
  void OnRtpPacketSend(RtpPacket rtp_packet);
  void OnReceiveRtcpFeedback(RtcpPacketPtr rtcp_packet);
  void OnReceiveReceiverReport(ReceiverReport* report);
  uint32_t GetPacingRate() { return 0; }

 private:
  void OnReceiveQuicAckFeedback(QuicAckFeedbackPacket* feedback);
  void RemoveOldSendPacket();

 private:
  // uv_loop
  UvLoop* uv_loop_{ nullptr };

  // quic send algorithm interface
  bool is_app_limit_{false};
  quic::SendAlgorithmInterface* send_algorithm_interface_{nullptr};
  quic::QuicClock* clock_{nullptr};
  quic::RttStats* rtt_stats_{nullptr};
  quic::QuicUnackedPacketMap* unacked_packet_map_{nullptr};
  quic::QuicRandom* random_{nullptr};
  quic::QuicConnectionStats* connection_stats_{nullptr};
  quic::AckedPacketVector acked_packets_;
  quic::LostPacketVector losted_packets_;
  quic::QuicByteCount bytes_in_flight_{0u};
  std::map<uint16_t, SendPacketInfo> has_send_map_;
  std::vector<SendPacketInfo> feedback_lost_no_count_packet_vec_;
  int64_t transport_rtt_{0u};
  uint16_t largest_acked_seq_{0u};
  int32_t cwnd_{6000u};
};
} // namespace bifrost

#endif  //_QUICSENDALGORITHMADAPTER_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 10:48 上午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_send_algorithm.h
 * @description : TODO
 *******************************************************/

#ifndef _BIFROST_SEND_ALGORITHM_H
#define _BIFROST_SEND_ALGORITHM_H

#include <memory>

#include "bifrost_send_algorithm_interface.h"
#include "quiche/quic/core/quic_types.h"

namespace bifrost {
typedef std::shared_ptr<BifrostSendAlgorithmInterface>
    BifrostSendAlgorithmInterfacePtr;

class BifrostSendAlgorithmManager {
 public:
  BifrostSendAlgorithmManager(quic::CongestionControlType congestion_algorithm_type,
                       UvLoop** uv_loop);
  ~BifrostSendAlgorithmManager();

 public:
  void OnRtpPacketSend(RtpPacket rtp_packet, int64_t now);
  void OnReceiveRtcpFeedback(const FeedbackRtpPacket* fb);
  void OnReceiveReceiverReport(ReceiverReport* report);
  void UpdateRtt(float rtt);
 private:
  BifrostSendAlgorithmInterfacePtr algorithm_interface_;
};

}  // namespace bifrost

#endif  //_BIFROST_SEND_ALGORITHM_H

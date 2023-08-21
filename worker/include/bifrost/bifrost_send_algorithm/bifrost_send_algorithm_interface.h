/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 3:53 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_send_algorithm_interface.h
 * @description : TODO
 *******************************************************/

#ifndef _BIFROST_SEND_ALGORITHM_INTERFACE_H
#define _BIFROST_SEND_ALGORITHM_INTERFACE_H
#include "common.h"
#include "rtcp_feedback.h"
#include "rtcp_rr.h"

namespace bifrost {
class BifrostSendAlgorithmInterface {
 public:
  virtual void OnRtpPacketSend(RtpPacket rtp_packet, int64_t now) = 0;

  virtual void OnReceiveRtcpFeedback(FeedbackRtpPacket* fb) = 0;
  virtual void OnReceiveReceiverReport(webrtc::RTCPReportBlock report,
                                       float rtt, int64_t nowMs) = 0;
  virtual void UpdateRtt(float rtt) = 0;
  virtual uint32_t GetPacingRate() = 0;
};
}  // namespace bifrost
#endif  //_BIFROST_SEND_ALGORITHM_INTERFACE_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : player.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_PLAYER_H
#define WORKER_PLAYER_H

#include <modules/rtp_rtcp/include/remote_ntp_time_estimator.h>
#include <modules/rtp_rtcp/source/rtp_format.h>
#include <modules/rtp_rtcp/source/rtp_format_h264.h>
#include <modules/video_coding/receiver.h>

#include "bifrost/bifrost_send_algorithm/tcc_server.h"
#include "bifrost/bifrost_send_algorithm/webrtc_clock_adapter.h"
#include "bifrost/experiment_manager/experiment_manager.h"
#include "nack.h"
#include "rtcp_rr.h"
#include "rtcp_sr.h"
#include "setting.h"

namespace bifrost {
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<Nack> NackPtr;
typedef std::shared_ptr<TransportCongestionControlServer>
    TransportCongestionControlServerPtr;
class Player : public UvTimer::Listener,
               public TransportCongestionControlServer::Observer {
 public:
  class Observer {
   public:
    virtual void OnPlayerSendPacket(RtcpPacketPtr packet,
                                    const struct sockaddr* remote_addr) = 0;
  };

 public:
  Player(const struct sockaddr* remote_addr, UvLoop** uv_loop,
         Observer* observer, uint32_t ssrc, uint8_t number,
         ExperimentManagerPtr& experiment_manager);
  ~Player() {
    this->nack_.reset();
    delete this->receiver_;
    delete this->timing_;
    delete this->clock_;
    delete this->depacketizer_;
    delete this->decoder_timer_;
  }

  // UvTimer
  void OnTimer(UvTimer* timer);

  // TransportCongestionControlServer::Observer
  void OnTransportCongestionControlServerSendRtcpPacket(
      TransportCongestionControlServer* tccServer,
      RtcpPacketPtr packet) override {
    packet->Serialize(Buffer);
    observer_->OnPlayerSendPacket(packet, udp_remote_address_.get());
  }

  void OnReceiveRtpPacket(RtpPacketPtr packet);

  void OnSendRtcpNack(RtcpPacketPtr packet) {
    observer_->OnPlayerSendPacket(packet, udp_remote_address_.get());
  }

  void OnReceiveSenderReport(SenderReport* report);

  ReceiverReport* GetRtcpReceiverReport();

 private:
  bool UpdateSeq(uint16_t seq);
  uint32_t GetExpectedPackets() const {
    return (this->cycles_ + this->max_seq_) - this->base_seq_ + 1;
  }

 private:
  /* ------------ base ------------ */
  // observer
  Observer* observer_;
  // remote addr
  SockAddressPtr udp_remote_address_;
  // uv
  UvLoop* uv_loop_;
  // ssrc
  uint32_t ssrc_;
  // number
  uint8_t number_;
  // experiment manager
  ExperimentManagerPtr experiment_manager_;

  /* ------------ experiment ------------ */
  // sr
  uint32_t last_sr_timestamp_;
  uint32_t last_sender_repor_ts_;
  uint64_t last_sr_received_;
  uint64_t last_sender_report_ntp_ms_;
  // rr
  uint32_t receive_packet_count_{0u};
  uint16_t max_seq_{0u};   // Highest seq. number seen.
  uint32_t cycles_{0u};    // Shifted count of seq. number cycles.
  uint32_t base_seq_{0u};  // Base seq number.
  uint32_t bad_seq_{0u};   // Last 'bad' seq number + 1.
  uint32_t expected_prior_{0u};
  uint32_t received_prior_{0u};
  uint32_t jitter_count_{0u};
  uint32_t jitter_{0u};
  // nack
  NackPtr nack_;
  // tcc
  TransportCongestionControlServerPtr tcc_server_{nullptr};
  // clock
  WebRTCClockAdapter* clock_;
  // timing
  webrtc::VCMTiming* timing_;
  // VCMReceiver
  webrtc::VCMReceiver* receiver_;
  // RtpDepacketizer
  webrtc::RtpDepacketizer* depacketizer_;
  // decoder timer
  UvTimer* decoder_timer_;
  // ntp_estimator
  webrtc::RemoteNtpTimeEstimator ntp_estimator_;

  bool drop_frames_until_keyframe_{false};
  bool prefer_late_decoding_{false};

  // pre decode time
  uint64_t pre_decode_time_ = 0;
};
}  // namespace bifrost

#endif  // WORKER_PLAYER_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 8:23 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_pacer.h
 * @description : TODO
 *******************************************************/

#ifndef _BIFROST_PACER_H
#define _BIFROST_PACER_H

#include "bifrost/experiment_manager/experiment_data.h"
#include "common.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<ExperimentDataProducerInterface>
    ExperimentDataProducerInterfacePtr;

class BifrostPacer : public UvTimer::Listener {
 public:
  class Observer {
   public:
    virtual void OnPublisherSendPacket(RtpPacketPtr packet) = 0;
    virtual void OnPublisherSendRtcpPacket(CompoundPacketPtr packet) = 0;
  };

 public:
  BifrostPacer(uint32_t ssrc, UvLoop* uv_loop, Observer* observer);
  ~BifrostPacer() override;

 public:
  // UvTimer
  void OnTimer(UvTimer* timer) override;

 public:
  void set_pacing_rate(uint32_t pacing_rate) {
    pacing_rate_ =
        pacing_rate > MaxPacingDataLimit ? MaxPacingDataLimit : pacing_rate;
  }  // bps
  void set_pacing_congestion_windows(uint32_t congestion_windows) {
    pacing_congestion_windows_ = congestion_windows;
  }
  void set_bytes_in_flight(uint32_t bytes_in_flight) {
    bytes_in_flight_ = bytes_in_flight;
  }
  void set_pacing_transfer_time(uint32_t pacing_transfer_time) {
    pacing_transfer_time_ = pacing_transfer_time;
  }
  void NackReadyToSendPacket(RtpPacketPtr packet) {
    this->ready_send_vec_.push_back(packet);
  }
  uint32_t get_pacing_packet_count() {
    auto tmp = pacing_packet_count_;
    pacing_packet_count_ = 0;
    return tmp;
  }
  uint32_t get_pacing_bytes() { return pacing_bytes_; }
  uint32_t get_pacing_bitrate_bps() { return pacing_bitrate_bps_; }

 private:
  // observer
  Observer* observer_;

  // fake date
  ExperimentDataProducerInterfacePtr data_producer_;
  uint16_t tcc_seq_{0u};

  // ready send
  std::vector<RtpPacketPtr> ready_send_vec_;

  // statistics
  uint32_t pacing_bytes_;
  uint32_t pacing_bitrate_;
  uint32_t pacing_bitrate_bps_;
  uint32_t pacing_packet_count_;

  // timer
  UvTimer* statistics_timer_;
  UvTimer* create_timer_;
  UvTimer* pacer_timer_;
  uint32_t pacing_rate_;
  uint32_t pacing_congestion_windows_; // bbr会使用拥塞窗口
  uint32_t bytes_in_flight_; // quic 使用统计飞行数据
  uint32_t pacing_transfer_time_; // quic 根据发送码率计算出发送间隔
  uint16_t pacer_timer_interval_{0u};
  int32_t pre_remainder_bytes_{0u};

  // max pacing limit
  static uint32_t MaxPacingDataLimit;
};
}  // namespace bifrost

#endif  //_BIFROST_PACER_H

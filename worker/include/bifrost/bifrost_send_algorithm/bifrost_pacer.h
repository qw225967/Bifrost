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
    pacing_rate_ = pacing_rate;
  }  // bps
  void NackReadyToSendPacket(RtpPacketPtr packet) {
    this->ready_send_vec_.push_back(packet);
  }

 private:
  // observer
  Observer* observer_;

  // fake date
  ExperimentDataProducerInterfacePtr data_producer_;
  uint16_t tcc_seq_{0u};

  // ready send
  std::vector<RtpPacketPtr> ready_send_vec_;

  // timer
  UvTimer* create_timer_;
  UvTimer* pacer_timer_;
  uint32_t pacing_rate_;
  uint16_t pacer_timer_interval_{0u};
  int32_t pre_remainder_bytes_{0u};
};
}  // namespace bifrost

#endif  //_BIFROST_PACER_H

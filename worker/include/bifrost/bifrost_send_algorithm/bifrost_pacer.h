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
typedef std::shared_ptr<ExperimentDataProducerInterface> ExperimentDataProducerInterfacePtr;
class BifrostPacer : public UvTimer{
  BifrostPacer(uint32_t ssrc, UvLoop* uv_loop);
  ~BifrostPacer();
 public:
  void OnTimer(UvTimer* timer);

 public:
  void set_pacing_rate(uint32_t pacing_rate);

 private:
  // fake date
  ExperimentDataProducerInterfacePtr data_producer_;
  std::vector<RtpPacketPtr> ready_send_vec_;

  // timer
  UvTimer *pacer_timer_;
  uint16_t pacer_timer_interval_{ 0u };
};
}

#endif  //_BIFROST_PACER_H

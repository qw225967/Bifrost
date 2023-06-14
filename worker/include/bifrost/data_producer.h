/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 10:04 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : data_producer.h
 * @description : 生产假数据的接口类，也可以继承后封装真数据
 *******************************************************/

#ifndef WORKER_DATA_PRODUCER_H
#define WORKER_DATA_PRODUCER_H

#include "rtp_packet.h"
#include "uv_timer.h"
#include "common.h"

namespace bifrost {
typedef std::shared_ptr<UvTimer> UvTimerPtr;
class DataProducer : public UvTimer::Listener {
 public:
  DataProducer(uv_loop_t* loop);
  ~DataProducer();

  void OnTimer(UvTimer *timer) override;

 public:
 void RangeCreateData();

 private:
  UvTimerPtr producer_timer;
};
}  // namespace bifrost

#endif  // WORKER_DATA_PRODUCER_H

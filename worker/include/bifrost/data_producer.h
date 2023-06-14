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
#incldue "timer.h"
#include "uv.h"

namespace bifrost {
class DataProducer : public UvTimer {
 public:
  DataProducer(uv_loop_t* loop) : UvTimer::UvTimer() {}
  ~DataProducer() {}

 public:
  RangeCreateData() {}
};
}  // namespace bifrost

#endif  // WORKER_DATA_PRODUCER_H

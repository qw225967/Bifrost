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

#include <fstream>

#include "common.h"
#include "rtp_packet.h"

namespace bifrost {
class DataProducer {
 public:
  DataProducer();
  ~DataProducer();

 public:
  RtpPacketPtr CreateData();

 private:
  std::ifstream data_file_;
};
}  // namespace bifrost

#endif  // WORKER_DATA_PRODUCER_H

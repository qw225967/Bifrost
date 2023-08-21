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
#include "experiment_data.h"
namespace bifrost {
typedef std::shared_ptr<RtpPacket> RtpPacketPtr;
class FakeDataProducer : public ExperimentDataProducerInterface {
 public:
  FakeDataProducer(uint32_t ssrc);
  ~FakeDataProducer();

 public:
  RtpPacketPtr CreateData(uint32_t available);

 private:
  void GetRtpExtensions(RtpPacketPtr &packet);

 private:
  std::ifstream data_file_;

  uint32_t ssrc_;
  uint16_t sequence_;
};
}  // namespace bifrost

#endif  // WORKER_DATA_PRODUCER_H

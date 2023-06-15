/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 10:04 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : data_producer.cpp
 * @description : TODO
 *******************************************************/

#include "data_producer.h"

#include <fstream>

namespace bifrost {
DataProducer::DataProducer(uv_loop_t* loop) {
  this->producer_timer = std::make_shared<UvTimer>(this, loop);
  this->producer_timer->Start(1000, 1000);
}

void DataProducer::RangeCreateData() {}

DataProducer::~DataProducer() {
  this->producer_timer->Stop();
  this->producer_timer.reset();
}

void DataProducer::OnTimer(UvTimer* timer) {
  if (timer == this->producer_timer.get()) {
    std::cout << "[data producer] timer call" << std::endl;
  }
}

}  // namespace bifrost
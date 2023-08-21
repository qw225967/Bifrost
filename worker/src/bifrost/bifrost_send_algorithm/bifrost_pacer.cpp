/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 8:23 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_pacer.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost_send_algorithm/bifrost_pacer.h"
#include "experiment_manager/fake_data_producer.h"

namespace bifrost {

constexpr uint16_t DefaultPacingTimeInterval = 5u;

BifrostPacer::BifrostPacer(uint32_t ssrc, UvLoop* uv_loop)
    : pacer_timer_interval_(DefaultPacingTimeInterval) {
  // 1.数据生产者
  data_producer_ = std::make_shared<FakeDataProducer>(ssrc);

  // 2.发送定时器
  pacer_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  pacer_timer_->Start(pacer_timer_interval_);
}

BifrostPacer::~BifrostPacer() {}

void BifrostPacer::OnTimer(UvTimer* timer) {
  if (timer == pacer_timer_) {

    pacer_timer_->Start(pacer_timer_interval_);
  }
}
}  // namespace bifrost
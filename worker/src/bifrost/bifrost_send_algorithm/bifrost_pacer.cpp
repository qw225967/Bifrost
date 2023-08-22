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

constexpr uint16_t DefaultCreatePacketTimeInterval = 10u; // 每10ms创建3个包
constexpr uint16_t DefaultPacingTimeInterval = 5u;
const uint32_t InitialPacingGccBitrate = 600000u;

BifrostPacer::BifrostPacer(uint32_t ssrc, UvLoop* uv_loop, Observer* observer)
    : observer_(observer),
      pacer_timer_interval_(DefaultPacingTimeInterval),
      pacing_rate_(InitialPacingGccBitrate) {
  // 1.数据生产者
  data_producer_ = std::make_shared<FakeDataProducer>(ssrc);

  // 2.发送定时器
  pacer_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  pacer_timer_->Start(pacer_timer_interval_);

  create_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  create_timer_->Start(DefaultCreatePacketTimeInterval, DefaultCreatePacketTimeInterval);
}

BifrostPacer::~BifrostPacer() {
  delete pacer_timer_;
  delete create_timer_;
}

void BifrostPacer::OnTimer(UvTimer* timer) {
  if (timer == pacer_timer_) {
    int32_t interval_pacing_bytes =
        int32_t((pacing_rate_ / 1000) /* 转换ms */ *
                pacer_timer_interval_ /* 该间隔发送速率 */ /
                8 /* 转换bytes */) +
        pre_remainder_bytes_ /* 上个周期剩余bytes */;

    auto ite = ready_send_vec_.begin();
    while (ite != ready_send_vec_.end() && interval_pacing_bytes > 0) {
      auto packet = (*ite);
      // 发送时更新tcc拓展序号，nack的rtp和普通rtp序号是连续的
      packet->UpdateTransportWideCc01(this->tcc_seq_++);

      observer_->OnPublisherSendPacket(packet);
      interval_pacing_bytes -= int32_t(packet->GetSize());

      ite = ready_send_vec_.erase(ite);
    }
    pre_remainder_bytes_ = interval_pacing_bytes;

    pacer_timer_->Start(pacer_timer_interval_);
  }

  if (timer == create_timer_) {
    // 每10ms产生3个包
    for (int i=0; i<3; i++) {
      auto packet = this->data_producer_->CreateData();
      this->ready_send_vec_.push_back(packet);
    }
  }
}
}  // namespace bifrost
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
#include "experiment_manager/h264_file_data_producer.h"

namespace bifrost {

constexpr uint16_t DefaultCreatePacketTimeInterval = 10u;  // 每10ms创建3个包
constexpr uint16_t DefaultStatisticsTimerInterval = 1000u;  // 每1s统计一次
constexpr uint16_t DefaultPacingTimeInterval = 5u;
const uint32_t InitialPacingGccBitrate = 600000u;

uint32_t BifrostPacer::MaxPacingDataLimit = 1200000;

BifrostPacer::BifrostPacer(uint32_t ssrc, UvLoop* uv_loop, Observer* observer)
    : observer_(observer),
      pacer_timer_interval_(DefaultPacingTimeInterval),
      pacing_rate_(InitialPacingGccBitrate) {
  // 1.数据生产者
  //  data_producer_ = std::make_shared<FakeDataProducer>(ssrc);
  data_producer_ =
      std::make_shared<H264FileDataProducer>(ssrc, uv_loop->get_loop().get());

  // 2.发送定时器
  pacer_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  pacer_timer_->Start(pacer_timer_interval_);

  create_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  create_timer_->Start(DefaultCreatePacketTimeInterval,
                       DefaultCreatePacketTimeInterval);

  statistics_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  statistics_timer_->Start(DefaultStatisticsTimerInterval,
                           DefaultStatisticsTimerInterval);
}

BifrostPacer::~BifrostPacer() {
  delete pacer_timer_;
  delete create_timer_;
  delete statistics_timer_;
}

void BifrostPacer::OnTimer(UvTimer* timer) {
  if (timer == pacer_timer_) {
    if (this->pacing_congestion_windows_ > 0 && this->bytes_in_flight_ > 0 &&
        this->pacing_congestion_windows_ < this->bytes_in_flight_) {
    } else {
      int32_t interval_pacing_bytes =
          int32_t((pacing_rate_ * 1.25 /* 乘上每次间隔码率加减的损失 */ /
                   1000) /* 转换ms */
                  * pacer_timer_interval_ /* 该间隔发送速率 */ /
                  8 /* 转换bytes */) +
          pre_remainder_bytes_ /* 上个周期剩余bytes */;

      auto ite = ready_send_vec_.begin();
      while (ite != ready_send_vec_.end() && interval_pacing_bytes > 0) {
        auto packet = (*ite);
        // 发送时更新tcc拓展序号，nack的rtp和普通rtp序号是连续的
        if (packet->UpdateTransportWideCc01(this->tcc_seq_)) {
          this->tcc_seq_++;

          //          std::cout << "send seq:" << packet->GetSequenceNumber() <<
          //          std::endl;
          observer_->OnPublisherSendPacket(packet);
        }

        interval_pacing_bytes -= int32_t(packet->GetSize());

        // 统计相关
        pacing_bitrate_ += packet->GetSize() * 8;
        pacing_bytes_ += packet->GetSize();
        pacing_packet_count_++;

        ite = ready_send_vec_.erase(ite);
      }
      pre_remainder_bytes_ = interval_pacing_bytes;
    }
    pacer_timer_->Start(pacer_timer_interval_);
  }

  if (timer == create_timer_) {
    // 每10ms产生3次
    for (int i = 0; i < 3; i++) {
      auto packet = this->data_producer_->CreateData();
      if (packet == nullptr) continue;
      auto size = packet->GetSize();
      this->ready_send_vec_.push_back(packet);
    }
  }

  if (timer == statistics_timer_) {
    pacing_bitrate_bps_ = pacing_bitrate_;
    pacing_bitrate_ = 0;
  }
}
}  // namespace bifrost
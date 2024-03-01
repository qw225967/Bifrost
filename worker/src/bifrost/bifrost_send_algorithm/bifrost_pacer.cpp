/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 8:23 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_pacer.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost_send_algorithm/bifrost_pacer.h"

#include <modules/rtp_rtcp/source/rtp_header_extension_size.h>
#include <modules/rtp_rtcp/source/rtp_packet_to_send.h>

#include "bifrost_send_algorithm/webrtc_clock_adapter.h"
#include "experiment_manager/fake_data_producer.h"
#include "experiment_manager/h264_file_data_producer.h"

namespace bifrost {

constexpr uint16_t DefaultCreatePacketTimeInterval = 10u;  // 每10ms创建3个包
constexpr uint16_t DefaultStatisticsTimerInterval = 1000u;  // 每1s统计一次
constexpr uint16_t DefaultPacingTimeInterval = 5u;
const uint32_t InitialPacingGccBitrate =
    400000u;  // 配合当前测试的码率一半左右开始探测 780

uint32_t BifrostPacer::MaxPacingDataLimit =
    700000;  // 当前测试的h264码率平均780kbps，因此限制最大为780

BifrostPacer::BifrostPacer(uint32_t ssrc, uint32_t flexfec_ssrc,
                           UvLoop* uv_loop, Observer* observer)
    : uv_loop_(uv_loop),
      observer_(observer),
      pacer_timer_interval_(DefaultPacingTimeInterval),
      pacing_rate_(InitialPacingGccBitrate) {
  // 1.数据生产者
#ifdef USE_FAKE_DATA_PRODUCER
  data_producer_ = std::make_shared<FakeDataProducer>(ssrc);
#else
  data_producer_ = std::make_shared<H264FileDataProducer>(ssrc, uv_loop);
#endif

  // 2.发送定时器
  pacer_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  pacer_timer_->Start(pacer_timer_interval_);

  create_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  create_timer_->Start(DefaultCreatePacketTimeInterval,
                       DefaultCreatePacketTimeInterval);

  statistics_timer_ = new UvTimer(this, uv_loop->get_loop().get());
  statistics_timer_->Start(DefaultStatisticsTimerInterval,
                           DefaultStatisticsTimerInterval);

#ifdef USE_FLEX_FEC_PROTECT
  // flexfec sender new
  std::vector<webrtc::RtpExtension> vec_ext;
  rtc::ArrayView<const webrtc::RtpExtensionSize> size;
  clock_ = new WebRTCClockAdapter(uv_loop);
  flexfec_sender_ = std::make_unique<webrtc::FlexfecSender>(
      110, flexfec_ssrc, ssrc, "", vec_ext, size, nullptr, clock_);

  loss_prot_logic_ =
      std::make_unique<webrtc::media_optimization::VCMLossProtectionLogic>(
          clock_->TimeInMilliseconds());

  // 开启保护模式，nack+fec
  this->SetProtectionMethod(true, true);
#endif
}

BifrostPacer::~BifrostPacer() {
  delete pacer_timer_;
  delete create_timer_;
  delete statistics_timer_;
  delete clock_;
}

void BifrostPacer::SetProtectionMethod(bool enable_fec, bool enable_nack) {
  webrtc::media_optimization::VCMProtectionMethodEnum method(
      webrtc::media_optimization::kNone);
  if (enable_fec && enable_nack) {
    method = webrtc::media_optimization::kNackFec;
  } else if (enable_nack) {
    method = webrtc::media_optimization::kNack;
  } else if (enable_fec) {
    method = webrtc::media_optimization::kFec;
  }

  loss_prot_logic_->SetMethod(method);
}

void BifrostPacer::UpdateFecRates(uint8_t fraction_lost,
                                  int64_t round_trip_time_ms) {
  float target_bitrate_kbps =
      static_cast<float>(this->target_bitrate_) / 1000.f;

  // Sanity check.
  if (this->last_pacing_frame_rate_ < 1.0) {
    this->last_pacing_frame_rate_ = 1.0;
  }

  {
    this->loss_prot_logic_->UpdateBitRate(target_bitrate_kbps);
    this->loss_prot_logic_->UpdateRtt(round_trip_time_ms);
    // Update frame rate for the loss protection logic class: frame rate should
    // be the actual/sent rate.
    loss_prot_logic_->UpdateFrameRate(last_pacing_frame_rate_);
    // Returns the filtered packet loss, used for the protection setting.
    // The filtered loss may be the received loss (no filter), or some
    // filtered value (average or max window filter).
    // Use max window filter for now.
    webrtc::media_optimization::FilterPacketLossMode filter_mode =
        webrtc::media_optimization::kMaxFilter;
    uint8_t packet_loss_enc = loss_prot_logic_->FilteredLoss(
        clock_->TimeInMilliseconds(), filter_mode, fraction_lost);
    // For now use the filtered loss for computing the robustness settings.
    loss_prot_logic_->UpdateFilteredLossPr(packet_loss_enc);
    if (loss_prot_logic_->SelectedType() == webrtc::media_optimization::kNone) {
      return;
    }
    // Update method will compute the robustness settings for the given
    // protection method and the overhead cost
    // the protection method is set by the user via SetVideoProtection.
    loss_prot_logic_->UpdateMethod();
    // Get the bit cost of protection method, based on the amount of
    // overhead data actually transmitted (including headers) the last
    // second.
    // Get the FEC code rate for Key frames (set to 0 when NA).
    key_fec_params_.fec_rate =
        loss_prot_logic_->SelectedMethod()->RequiredProtectionFactorK();
    // Get the FEC code rate for Delta frames (set to 0 when NA).
    delta_fec_params_.fec_rate =
        loss_prot_logic_->SelectedMethod()->RequiredProtectionFactorD();
    // The RTP module currently requires the same |max_fec_frames| for both
    // key and delta frames.
    delta_fec_params_.max_fec_frames =
        loss_prot_logic_->SelectedMethod()->MaxFramesFec();
    key_fec_params_.max_fec_frames =
        loss_prot_logic_->SelectedMethod()->MaxFramesFec();
  }
  // Set the FEC packet mask type. |kFecMaskBursty| is more effective for
  // consecutive losses and little/no packet re-ordering. As we currently
  // do not have feedback data on the degree of correlated losses and packet
  // re-ordering, we keep default setting to |kFecMaskRandom| for now.
  delta_fec_params_.fec_mask_type = webrtc::kFecMaskRandom;
  key_fec_params_.fec_mask_type = webrtc::kFecMaskRandom;

  // 随便设置一个
  if (flexfec_sender_) {
    flexfec_sender_->SetFecParameters(delta_fec_params_);
    std::cout << "fec rate:" << delta_fec_params_.fec_rate
              << ", max frame rate:" << delta_fec_params_.max_fec_frames
              << ", fraction lost:" << uint32_t(fraction_lost)
              << std::endl;
  }
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

          if (packet->HasMarker()) count_pacing_frame_rate_++;

          observer_->OnPublisherSendPacket(packet);
        }

        // fec
        if (packet->GetPayloadType() == 110)
          observer_->OnPublisherSendPacket(packet);

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
    delta_fec_params_.fec_rate = 255;
    delta_fec_params_.max_fec_frames = 1;
    flexfec_sender_->SetFecParameters(delta_fec_params_);
    // 每10ms产生3次
    for (int i = 0; i < 5; i++) {
      auto packet = this->data_producer_->CreateData();
      if (packet == nullptr) continue;

      if (flexfec_sender_ && loss_prot_logic_ && clock_) {
        // TODO:重构packet部分，现在打包逻辑非常混乱，内存管理不当
        auto *buffer = new uint8_t[packet->GetSize()];
        memcpy(buffer, packet->GetData(), packet->GetSize());
        webrtc::RtpPacketToSend webrtc_packet(nullptr);
        webrtc_packet.Parse(buffer, packet->GetSize());
        flexfec_sender_->AddRtpPacketAndGenerateFec(webrtc_packet);
        auto vec_s = flexfec_sender_->GetFecPackets();
        if (!vec_s.empty()) {
          for (auto iter = vec_s.begin(); iter != vec_s.end(); iter++) {
            auto len = (*iter)->size();
            auto *packet_data = new uint8_t[len];
            memcpy(packet_data, (*iter)->data(), len);
            RtpPacketPtr rtp_packet =
                RtpPacket::Parse(packet_data, (*iter)->size());

            rtp_packet->SetPayloadDataPtr(&packet_data);

            this->ready_send_vec_.push_back(rtp_packet);
          }
        }
      }

      this->ready_send_vec_.push_back(packet);
    }
  }

  if (timer == statistics_timer_) {
    pacing_bitrate_bps_ = pacing_bitrate_;
    pacing_bitrate_ = 0;
    last_pacing_frame_rate_ = count_pacing_frame_rate_;
    count_pacing_frame_rate_ = 0;
  }
}
}  // namespace bifrost
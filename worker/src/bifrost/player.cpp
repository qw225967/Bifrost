/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : player.cpp
 * @description : TODO
 *******************************************************/

//#define USE_NS3_TEST 1

#include "player.h"

#include <modules/rtp_rtcp/source/rtp_packet_received.h>
#include <modules/video_coding/encoded_frame.h>
#include <modules/video_coding/frame_object.h>

#include <ctime>

#include <ctime>

namespace bifrost {
static constexpr uint16_t MaxDropout{3000};
static constexpr uint16_t MaxMisorder{1500};
static constexpr uint32_t RtpSeqMod{1 << 16};
static constexpr size_t ScoreHistogramLength{24};
const uint16_t DecoderIntervalMs{10u};
const int64_t kMaxWaitTime = 10000;
constexpr int kPacketBufferStartSize = 512;
constexpr int kPacketBufferMaxSize = 2048;

Player::Player(const struct sockaddr* remote_addr, UvLoop** uv_loop,
               Observer* observer, uint32_t ssrc, uint8_t number,
               ExperimentManagerPtr& experiment_manager)
    : uv_loop_(*uv_loop),
      observer_(observer),
      ssrc_(ssrc),
      experiment_manager_(experiment_manager),
      number_(number),
      clock_(new WebRTCClockAdapter(this->uv_loop_)),
      ntp_estimator_(clock_) {
  std::cout << "player experiment manager:" << experiment_manager << std::endl;

#ifdef USE_NS3_TEST
  // 设置代理ip、端口
  struct sockaddr_storage temp_addr;
  int family = IP::get_family("10.100.0.100");
  switch (family) {
    case AF_INET: {
      int err = uv_ip4_addr("10.100.0.100", 0,
                            reinterpret_cast<struct sockaddr_in*>(&temp_addr));
      std::cout << "[proxy] remote uv_ip4_addr" << std::endl;
      if (err != 0)
        std::cout << "[proxy] remote uv_ip4_addr() failed: " << uv_strerror(err)
                  << std::endl;

      (reinterpret_cast<struct sockaddr_in*>(&temp_addr))->sin_port =
          htons(8887);
      break;
    }
  }
  auto temp_sockaddr = reinterpret_cast<struct sockaddr*>(&temp_addr);
  this->udp_remote_address_ = std::make_shared<sockaddr>(*temp_sockaddr);
#else
  // 1.remote address set
  this->udp_remote_address_ = std::make_shared<sockaddr>(*remote_addr);
#endif

  // 2.nack
  this->nack_ = std::make_shared<Nack>(ssrc, uv_loop, this);

  // 3.tcc server
  this->tcc_server_ = std::make_shared<TransportCongestionControlServer>(
      this, MtuSize, &this->uv_loop_);

  // 4.timing
  timing_ = new webrtc::VCMTiming(clock_);

  // 5.receiver
#ifdef USE_VCM_RECEIVER
  receiver_ = new webrtc::VCMReceiver(timing_, clock_);
  receiver_->SetNackSettings(webrtc::kMaxNumberOfFrames,
                             webrtc::kMaxNumberOfFrames, 0);

  // 6.decoder timer
  decoder_timer_ = new UvTimer(this, uv_loop_->get_loop().get());
  decoder_timer_->Start(DecoderIntervalMs, DecoderIntervalMs);
#endif

#ifdef USE_VCM_PACKET_BUFFER
  packet_buffer_ = webrtc::video_coding::PacketBuffer::Create(
      clock_, kPacketBufferStartSize, kPacketBufferMaxSize, this);
  reference_finder_ =
      std::make_unique<webrtc::video_coding::RtpFrameReferenceFinder>(this);
#endif
  // 7.depacketizer
  depacketizer_ = new webrtc::RtpDepacketizerH264();

  // 8.fec receiver
  flexfec_receiver_ =
      std::make_unique<webrtc::FlexfecReceiver>(ssrc + 1, ssrc, this);
}

void Player::OnAssembledFrame(
    std::unique_ptr<webrtc::video_coding::RtpFrameObject> frame) {
#ifdef USE_VCM_PACKET_BUFFER
  auto lase_seq = frame->last_seq_num();

  reference_finder_->ManageFrame(std::move(frame));
//  this->packet_buffer_->ClearTo(lase_seq);
//  this->reference_finder_->ClearTo(lase_seq);
#endif
}

void Player::OnCompleteFrame(
    std::unique_ptr<webrtc::video_coding::EncodedFrame> frame) {
  time_t currentTime = time(nullptr);
  struct tm* localTime = localtime(&currentTime);
  std::cout << "[" << localTime->tm_hour << ":" << localTime->tm_min << ":"
            << localTime->tm_sec << "]"
            << "reference finder OnCompleteFrame frame interval to decode:"
            << frame->id.picture_id << ", interval:"
            << this->uv_loop_->get_time_ms_int64() - pre_decode_time_
            << std::endl;

  pre_decode_time_ = this->uv_loop_->get_time_ms_int64();
}

bool Player::UpdateSeq(uint16_t seq) {
  uint16_t udelta = seq - this->max_seq_;

  // If the new packet sequence number is greater than the max seen but not
  // "so much bigger", accept it.
  // NOTE: udelta also handles the case of a new cycle, this is:
  //    maxSeq:65536, seq:0 => udelta:1
  if (udelta < MaxDropout) {
    // In order, with permissible gap.
    if (seq < this->max_seq_) {
      // Sequence number wrapped: count another 64K cycle.
      this->cycles_ += RtpSeqMod;
    }

    this->max_seq_ = seq;
  }
  // Too old packet received (older than the allowed misorder).
  // Or to new packet (more than acceptable dropout).
  else if (udelta <= RtpSeqMod - MaxMisorder) {
    // The sequence number made a very large jump. If two sequential packets
    // arrive, accept the latter.
    if (seq == this->bad_seq_) {
      // Initialize/reset RTP counters.
      this->base_seq_ = seq;
      this->max_seq_ = seq;
      this->bad_seq_ = RtpSeqMod + 1;  // So seq == badSeq is false.
    } else {
      this->bad_seq_ = (seq + 1) & (RtpSeqMod - 1);

      return false;
    }
  }
  // Acceptable misorder.
  else {
    // Do nothing.
  }

  return true;
}

void Player::OnRecoveredPacket(const uint8_t* packet, size_t length) {
  auto recover_packet = std::make_shared<RtpPacket>(packet, length);
  if (!recover_packet) {
    std::cout << "recive rtp packet fec recover packet data is not a valid RTP "
                 "packet length:"
              << length << std::endl;
    return;
  }

  this->OnReceiveRtpPacket(recover_packet, true);
}

void Player::OnReceiveRtpPacket(RtpPacketPtr packet, bool is_recover) {
  receive_packet_count_++;

  bool is_fec_packet = packet->GetPayloadType() == 110;

  // 转换信息
  // 转换webrtc包
  webrtc::RtpPacketReceived parsed_packet;
  if (!parsed_packet.Parse(packet->GetData(), packet->GetSize())) {
    return;
  }
  parsed_packet.set_recovered(is_recover);

  if (flexfec_receiver_) {
    flexfec_receiver_->OnRtpPacket(parsed_packet);

    if (is_fec_packet) {
      if (fec_nack_ == nullptr) {
        fec_nack_ =
            std::make_shared<Nack>(packet->GetSsrc(), &this->uv_loop_, this);
      }
      fec_nack_->OnReceiveRtpPacket(packet);
      return;
    }
  }

  this->UpdateSeq(packet->GetSequenceNumber());
  this->nack_->OnReceiveRtpPacket(packet);
  this->tcc_server_->IncomingPacket(this->uv_loop_->get_time_ms_int64(),
                                    packet.get());
  this->tcc_server_->QuicCountIncomingPacket(
      this->uv_loop_->get_time_ms_int64(), packet.get());

  webrtc::RtpDepacketizer::ParsedPayload parsed_payload;
  if (!depacketizer_->Parse(&parsed_payload, parsed_packet.payload().data(),
                            parsed_packet.payload().size())) {
    return;
  }

  // 解rtp头
  webrtc::RTPHeader header;
  parsed_packet.GetHeader(&header);
  parsed_payload.video_header().is_last_packet_in_frame |= header.markerBit;
#ifdef USE_VCM_RECEIVER
  // 转vcm包
  const webrtc::VCMPacket vcm_packet(
      const_cast<uint8_t*>(parsed_packet.payload().data()),
      parsed_packet.payload_size(), header, parsed_payload.video_header(),
      /*ntp_estimator_.Estimate(header.timestamp)*/ 0,
      clock_->TimeInMilliseconds());

  //  std::cout << Byte::bytes_to_hex(parsed_packet.payload().data(),
  //                                  parsed_packet.payload().size())
  //            << std::endl;

  auto ret = this->receiver_->InsertPacket(vcm_packet);
  if (ret == /*VCM_FLUSH_INDICATOR*/ 4) {
    drop_frames_until_keyframe_ = true;
  }
#endif

#ifdef USE_VCM_PACKET_BUFFER
  // 转vcm包
  webrtc::VCMPacket vcm_packet(
      const_cast<uint8_t*>(parsed_packet.payload().data()),
      parsed_packet.payload_size(), header, parsed_payload.video_header(),
      /*ntp_estimator_.Estimate(header.timestamp)*/ 0,
      clock_->TimeInMilliseconds());

  if (this->packet_buffer_->InsertPacket(&vcm_packet)) {
  }
#endif
}

void Player::OnReceiveSenderReport(SenderReport* report) {
  // test
#ifdef USE_VCM_RECEIVER
  auto info = receiver_->GetTimingInfo();
  if (info.has_value()) std::cout << info->ToString() << std::endl;
#endif

  this->last_sr_received_ = this->uv_loop_->get_time_ms();
  this->last_sr_timestamp_ = report->GetNtpSec() << 16;
  this->last_sr_timestamp_ += report->GetNtpFrac() >> 16;

  // Update info about last Sender Report.
  Time::Ntp ntp;  // NOLINT(cppcoreguidelines-pro-type-member-init)

  ntp.seconds = report->GetNtpSec();
  ntp.fractions = report->GetNtpFrac();

  this->last_sender_report_ntp_ms_ = Time::Ntp2TimeMs(ntp);
  this->last_sender_repor_ts_ = report->GetRtpTs();

  ntp_estimator_.UpdateRtcpTimestamp(100, ntp.seconds, ntp.fractions,
                                     report->GetRtpTs());
}

ReceiverReport* Player::GetRtcpReceiverReport() {
  auto* report = new ReceiverReport();

  report->SetSsrc(this->ssrc_);

  // Calculate Packets Expected and Lost.
  auto expected = this->GetExpectedPackets();

  int32_t packet_lost_ = 0;
  if (expected > this->receive_packet_count_)
    packet_lost_ = expected - this->receive_packet_count_;

  // Calculate Fraction Lost.
  uint32_t expectedInterval = expected - this->expected_prior_;

  this->expected_prior_ = expected;

  uint32_t receivedInterval =
      this->receive_packet_count_ - this->received_prior_;

  this->received_prior_ = this->receive_packet_count_;

  int32_t lostInterval = expectedInterval - receivedInterval;

  uint8_t fraction_lost = 0;
  if (expectedInterval != 0 && lostInterval > 0)
    fraction_lost =
        std::round((static_cast<double>(lostInterval << 8) / expectedInterval));

  report->SetTotalLost(packet_lost_);
  report->SetFractionLost(fraction_lost);

  // Fill the rest of the report.
  report->SetLastSeq(static_cast<uint32_t>(this->max_seq_) + this->cycles_);

  report->SetJitter(this->jitter_ / 1000);  // 换算单位

  if (this->jitter_count_ < 5) {
    report->SetJitter(0);
    jitter_count_++;
  }

  if (this->last_sr_received_ != 0) {
    // Get delay in milliseconds.
    auto delayMs = static_cast<uint32_t>(this->uv_loop_->get_time_ms() -
                                         this->last_sr_received_);
    // Express delay in units of 1/65536 seconds.
    uint32_t dlsr = (delayMs / 1000) << 16;

    dlsr |= uint32_t{(delayMs % 1000) * 65536 / 1000};

    report->SetDelaySinceLastSenderReport(dlsr);
    report->SetLastSenderReport(this->last_sr_timestamp_);
  } else {
    report->SetDelaySinceLastSenderReport(0);
    report->SetLastSenderReport(0);
  }
  return report;
}

void Player::OnTimer(UvTimer* timer) {
  if (timer == decoder_timer_) {
#ifdef USE_VCM_RECEIVER
    bool prefer_late_decoding = true;
    webrtc::VCMEncodedFrame* frame =
        this->receiver_->FrameForDecoding(kMaxWaitTime, prefer_late_decoding_);

    if (!frame) return;

    time_t currentTime = time(nullptr);
    struct tm* localTime = localtime(&currentTime);

    std::cout << "[" << localTime->tm_hour << ":" << localTime->tm_min << ":"
              << localTime->tm_sec << "]"
              << "frame interval to decode:"
              << this->uv_loop_->get_time_ms_int64() - pre_decode_time_
              << std::endl;

    pre_decode_time_ = this->uv_loop_->get_time_ms_int64();

    bool drop_frame = false;
    {
      if (drop_frames_until_keyframe_) {
        // Still getting delta frames, schedule another keyframe request as if
        // decode failed.
        if (frame->FrameType() != webrtc::VideoFrameType::kVideoFrameKey) {
          drop_frame = true;
        } else {
          drop_frames_until_keyframe_ = false;
        }
      }
    }

    if (drop_frame) {
      receiver_->ReleaseFrame(frame);
      prefer_late_decoding_ = true;
      return;
    }

    this->timing_->UpdateCurrentDelay(frame->RenderTimeMs(),
                                      clock_->TimeInMilliseconds());

    this->receiver_->ReleaseFrame(frame);
#endif
  }
}

}  // namespace bifrost

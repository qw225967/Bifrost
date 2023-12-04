/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/9/14 3:38 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : h264_file_this->buffer__producer.cpp
 * @description : TODO
 *******************************************************/

#include "experiment_manager/h264_file_data_producer.h"

#include <modules/rtp_rtcp/source/rtp_format.h>

namespace bifrost {

const uint32_t IntervalReadFrameMs = 66u;

H264FileDataProducer::H264FileDataProducer(uint32_t ssrc, uv_loop_t *loop) {
  this->h264_data_file_.open("../source_file/test.h264",
                             std::ios::in | std::ios::binary);

  // TODO:此处一次性转存文件，后续改成分片读取
  if (this->h264_data_file_.is_open()) {
    this->h264_data_file_.seekg(0, std::ios::end);
    this->size_ = this->h264_data_file_.tellg();
  }
  this->buffer_ = new uint8_t[this->size_];
  memset(this->buffer_, 0, this->size_);
  this->h264_data_file_.seekg(0);
  this->h264_data_file_.read((char *)this->buffer_, this->size_);

  this->h264_data_file_.close();

  this->read_frame_timer_ = new UvTimer(this, loop);
  this->read_frame_timer_->Start(IntervalReadFrameMs, IntervalReadFrameMs);
}

H264FileDataProducer::~H264FileDataProducer() {
  delete[] this->buffer_;
  delete this->read_frame_timer_;
}

NaluType H264FileDataProducer::PrintfH264Frame(int j, int nLen,
                                               int nFrameType) {
  int nForbiddenBit = (nFrameType >> 7) & 0x1;  //第1位禁止位，值为1表示语法出错
  int nReference_idc = (nFrameType >> 5) & 0x03;  //第2~3位为参考级别
  int nType = nFrameType & 0x1f;                  //第4~8为是nal单元类型

  switch (nReference_idc) {
    case NALU_PRIORITY_DISPOSABLE:
      std::cout << "DISPOS ";
      break;
    case NALU_PRIRITY_LOW:
      std::cout << "LOW ";
      break;
    case NALU_PRIORITY_HIGH:
      std::cout << "HIGH ";
      break;
    case NALU_PRIORITY_HIGHEST:
      std::cout << "HIGHEST ";
      break;
  }

  switch (nType) {
    case NALU_TYPE_SLICE:
      std::cout << "SLICE ";
      break;
    case NALU_TYPE_DPA:
      std::cout << "DPA ";
      break;
    case NALU_TYPE_DPB:
      std::cout << "DPB ";
      break;
    case NALU_TYPE_DPC:
      std::cout << "DPC ";
      break;
    case NALU_TYPE_IDR:
      std::cout << "IDR ";
      break;
    case NALU_TYPE_SEI:
      std::cout << "SEI ";
      break;
    case NALU_TYPE_SPS:
      std::cout << "SPS ";
      break;
    case NALU_TYPE_PPS:
      std::cout << "PPS ";
      break;
    case NALU_TYPE_AUD:
      std::cout << "AUD ";
      break;
    case NALU_TYPE_EOSEQ:
      std::cout << "EOSEQ ";
      break;
    case NALU_TYPE_EOSTREAM:
      std::cout << "EOSTREAM ";
      break;
    case NALU_TYPE_FILL:
      std::cout << "FILL ";
      break;
  }

  std::cout << "frame rate:" << j << ", frame size:" << nLen << std::endl;
}

int H264FileDataProducer::GetH264FrameLen(int n_pos, size_t n_total_size,
                                          uint8_t *data) {
  int n_start = n_pos;
  while (n_start < n_total_size) {
    if (data[n_start] == 0x00 && data[n_start + 1] == 0x00 &&
        data[n_start + 2] == 0x01) {
      return n_start - n_pos;
    } else if (data[n_start] == 0x00 && data[n_start + 1] == 0x00 &&
               data[n_start + 2] == 0x00 && data[n_start + 3] == 0x01) {
      return n_start - n_pos;
    } else {
      n_start++;
    }
  }
  return n_total_size - n_pos;  //最后一帧。
}

RtpPacketPtr H264FileDataProducer::CreateData() { return nullptr; }

void H264FileDataProducer::ReadOneH264Frame() {
  if (this->read_offset < this->size_ - 4) {
    if (this->buffer_[this->read_offset] == 0x00 &&
        this->buffer_[this->read_offset + 1] == 0x00 &&
        this->buffer_[this->read_offset + 2] == 0x01) {
      this->frame_len_ = this->GetH264FrameLen(this->read_offset + 3,
                                               this->size_, this->buffer_);
      this->payload_ = this->buffer_ + this->read_offset + 4;

      this->frame_count_++;
      this->read_offset += 3;
    } else if (this->buffer_[this->read_offset] == 0x00 &&
               this->buffer_[this->read_offset + 1] == 0x00 &&
               this->buffer_[this->read_offset + 2] == 0x00 &&
               this->buffer_[this->read_offset + 3] == 0x01) {
      this->frame_len_ = this->GetH264FrameLen(this->read_offset + 4,
                                               this->size_, this->buffer_);
      this->payload_ = this->buffer_ + this->read_offset + 4;

      this->frame_count_++;
      this->read_offset += 4;
    } else {
      this->read_offset++;
    }
  }
}

void H264FileDataProducer::ReadWebRTCRtpPacketizer() {
  webrtc::RtpPacketizer::PayloadSizeLimits limits;
  limits.max_payload_len = 1400 - RtpPacket::HeaderSize;
  limits.first_packet_reduction_len = 1400 - RtpPacket::HeaderSize;
  limits.single_packet_reduction_len = 1400 - RtpPacket::HeaderSize;
  limits.last_packet_reduction_len = 1400 - RtpPacket::HeaderSize;

  absl::optional<webrtc::VideoCodecType> type = webrtc::kVideoCodecH264;

  webrtc::RTPVideoHeader packetize_video_header;
  packetize_video_header.video_type_header = webrtc::RTPVideoHeaderH264();

  auto nalu_type = this->PrintfH264Frame(this->frame_count_, this->frame_len_,
                                         this->payload_[0]);
  webrtc::VideoFrameType frame_type =
      webrtc::VideoFrameType::kVideoFrameDelta;
  if (nalu_type == NALU_TYPE_IDR) {
    frame_type = webrtc::VideoFrameType::kVideoFrameKey;
  }

  webrtc::RTPFragmentationHeader fragmentation;

  std::unique_ptr<webrtc::RtpPacketizer> packetizer =
      webrtc::RtpPacketizer::Create(
          type, rtc::MakeArrayView(this->payload_, this->frame_len_), limits,
          packetize_video_header, frame_type, &fragmentation);
}

void H264FileDataProducer::OnTimer(UvTimer *timer) {
  if (timer == this->read_frame_timer_) {
    this->ReadOneH264Frame();
    this->ReadWebRTCRtpPacketizer();
  }
}

}  // namespace bifrost
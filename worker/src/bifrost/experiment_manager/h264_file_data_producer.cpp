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
#include <modules/rtp_rtcp/source/rtp_packet_to_send.h>

namespace bifrost {

const uint32_t IntervalReadFrameMs = 10u; // 速度快一些多准备些数据（没能完全模拟采集的分布）
const uint32_t MaxPacketSize = 1100u;

H264FileDataProducer::H264FileDataProducer(uint32_t ssrc, uv_loop_t *loop)
    : ssrc_(ssrc) {
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
//      std::cout << "DISPOS ";
      break;
    case NALU_PRIRITY_LOW:
//      std::cout << "LOW ";
      break;
    case NALU_PRIORITY_HIGH:
//      std::cout << "HIGH ";
      break;
    case NALU_PRIORITY_HIGHEST:
//      std::cout << "HIGHEST ";
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

//    std::cout << "frame rate:" << j << ", frame size:" << nLen << std::endl;
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

RtpPacketPtr H264FileDataProducer::CreateData() {
  auto packet_ite = this->rtp_packet_vec_.begin();
  if (packet_ite != this->rtp_packet_vec_.end()) {
    auto packet = *packet_ite;
    rtp_packet_vec_.erase(packet_ite);
    return packet;
  }
  return nullptr;
}

bool H264FileDataProducer::ReadOneH264Frame() {
  if (this->read_offset < this->size_ - 4) {
    if (this->buffer_[this->read_offset] == 0x00 &&
        this->buffer_[this->read_offset + 1] == 0x00 &&
        this->buffer_[this->read_offset + 2] == 0x01) {
      this->frame_len_ = this->GetH264FrameLen(this->read_offset + 3,
                                               this->size_, this->buffer_);
      this->payload_ = this->buffer_ + this->read_offset;

      this->frame_count_++;
      this->read_offset += 3;

      return false;
    } else if (this->buffer_[this->read_offset] == 0x00 &&
               this->buffer_[this->read_offset + 1] == 0x00 &&
               this->buffer_[this->read_offset + 2] == 0x00 &&
               this->buffer_[this->read_offset + 3] == 0x01) {
      this->frame_len_ = this->GetH264FrameLen(this->read_offset + 4,
                                               this->size_, this->buffer_);
      this->payload_ = this->buffer_ + this->read_offset;

      this->frame_count_++;
      this->read_offset += 4;

      return false;
    } else {
      this->read_offset++;
    }
    return true;
  } else {
    std::cout << "read finish!" << std::endl;
  }

  return false;
}

// 将帧切割为一个又一个的NALU
void H264FileDataProducer::H264FrameAnnexbSplitNalu(
    uint8_t *data, uint32_t len,
    std::function<bool(uint8_t *data, uint32_t len, uint32_t start_code_len)>
        on_nalu) {
  uint8_t *start = data;
  /// found 0x00000001 or 0x000001 is a nalu
  while (len > 4) {
    /// start_code
    int start_code_len = 0;
    if (data[0] == 0 && data[1] == 0 && data[2] == 1) {  // TODO: 00 00 01
      start_code_len = 3;
    } else if (data[0] == 0 && data[1] == 0 && data[2] == 0 &&
               data[3] == 1) {  // TODO: 00 00 00 01
      start_code_len = 4;
    }
    /// 发现一个新的NALU
    /// NALU = StartCode(3,4) + NALU Header(1) + NALU Payload(n)
    if (start_code_len > 0) {
      if (data > start) {
        /// 回调上一个NALU，至少从第二个NALU开始
        if (on_nalu(start, data - start, start_code_len)) {
          start = data;
        } else {
          return;
        }
      }
      data += start_code_len;  //!< next
      len -= start_code_len;
    } else {
      data++;  //!< next
      len--;
    }
  }
  /// 当LEN小于4时，把剩下的字节也追加进去并回调
  data += len;
  if (data > start) {
    on_nalu(start, data - start, 0);
  }
}

void H264FileDataProducer::ReadWebRTCRtpPacketizer() {
  webrtc::RtpPacketizer::PayloadSizeLimits limits;
  limits.max_payload_len = MaxPacketSize - RtpPacket::HeaderSize;
  limits.first_packet_reduction_len = RtpPacket::HeaderSize;
  limits.single_packet_reduction_len = RtpPacket::HeaderSize;
  limits.last_packet_reduction_len = RtpPacket::HeaderSize;

  absl::optional<webrtc::VideoCodecType> type = webrtc::kVideoCodecH264;

  auto nalu_type = this->PrintfH264Frame(this->frame_count_, this->frame_len_,
                                         this->payload_[0]);

  int rtp_fragment_size = 0;
  std::vector<std::pair<size_t, size_t>>
      fragment_infos;  // pair 的第一个为 length，第二个位 offset
  this->H264FrameAnnexbSplitNalu(
      this->payload_, this->frame_len_,
      [&rtp_fragment_size, &fragment_infos](uint8_t *data, uint32_t len,
                                            uint32_t start_code_len) {
        std::pair<size_t, size_t> info;
        info.first = len - start_code_len;
        info.second = start_code_len;
        fragment_infos.push_back(info);

        rtp_fragment_size += 1;
        return true;
      });

  webrtc::RTPVideoHeader packetize_video_header;
  webrtc::RTPVideoHeaderH264 h264 = webrtc::RTPVideoHeaderH264();

  webrtc::VideoFrameType frame_type = webrtc::VideoFrameType::kVideoFrameDelta;
  if (nalu_type == NALU_TYPE_IDR) {
    frame_type = webrtc::VideoFrameType::kVideoFrameKey;
  } else if (nalu_type == NALU_TYPE_PPS || nalu_type == NALU_TYPE_SEI ||
             nalu_type == NALU_TYPE_SPS) {
    h264.packetization_mode = webrtc::H264PacketizationMode::SingleNalUnit;
  }
  packetize_video_header.video_type_header = h264;

  webrtc::RTPFragmentationHeader fragmentation;
  fragmentation.Resize(rtp_fragment_size);

  for (int i = 0; i < fragment_infos.size(); i++) {
    fragmentation.fragmentationLength[i] = fragment_infos[i].first;
    fragmentation.fragmentationOffset[i] = fragment_infos[i].second;
  }

  std::unique_ptr<webrtc::RtpPacketizer> packetizer =
      webrtc::RtpPacketizer::Create(
          type, rtc::MakeArrayView(this->payload_, this->frame_len_), limits,
          packetize_video_header, frame_type, &fragmentation);

  const size_t num_packets = packetizer->NumPackets();
  if (num_packets == 0) return;

  for (auto i = 0; i < num_packets; i++) {
    int expected_payload_capacity;
    // Choose right packet template:
    if (num_packets == 1) {
      expected_payload_capacity =
          limits.max_payload_len - limits.single_packet_reduction_len;
    } else if (i == 0) {
      expected_payload_capacity =
          limits.max_payload_len - limits.first_packet_reduction_len;
    } else if (i == num_packets - 1) {
      expected_payload_capacity =
          limits.max_payload_len - limits.last_packet_reduction_len;
    } else {
      expected_payload_capacity = limits.max_payload_len;
    }

    auto *packet =
        new webrtc::RtpPacketToSend(nullptr, expected_payload_capacity + RtpPacket::HeaderSize);
    if (!packetizer->NextPacket(packet)) return;

    packet->SetSequenceNumber(this->sequence_++);
    packet->SetPayloadType(101);
    packet->SetSsrc(this->ssrc_);
    packet->SetPayloadSize(expected_payload_capacity);

    // 转回bifrost的rtp格式
    auto len = packet->capacity();
    auto *payload_data = new uint8_t[len];
    memcpy(payload_data, packet->data(), len);
    RtpPacketPtr rtp_packet = RtpPacket::Parse(payload_data, len);
    // mediasoup parse 内部只new了包结构，没有new
    // payload空间，payload空间使用了一个共享的静态区域
    rtp_packet->SetPayloadDataPtr(&payload_data);

    this->GetRtpExtensions(rtp_packet);

    rtp_packet_vec_.push_back(rtp_packet);
  }
}

void H264FileDataProducer::GetRtpExtensions(RtpPacketPtr &packet) {
  static uint8_t buffer[4096];
  uint8_t extenLen = 2u;
  static std::vector<RtpPacket::GenericExtension> extensions;
  // This happens just once.
  if (extensions.capacity() != 24) extensions.reserve(24);

  extensions.clear();

  uint8_t *bufferPtr{buffer};
  // NOTE: Add value 0. The sending Transport will update it.
  uint16_t wideSeqNumber{0u};

  Byte::set_2_bytes(bufferPtr, 0, wideSeqNumber);
  extensions.emplace_back(static_cast<uint8_t>(7), extenLen, bufferPtr);
  packet->SetExtensions(2, extensions);
  packet->SetTransportWideCc01ExtensionId(7);
}

void H264FileDataProducer::OnTimer(UvTimer *timer) {
  if (timer == this->read_frame_timer_) {
    while (this->ReadOneH264Frame())
      ;
    this->ReadWebRTCRtpPacketizer();
  }
}

}  // namespace bifrost
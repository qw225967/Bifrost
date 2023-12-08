/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/9/6 5:15 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : video_file_data_producer.h
 * @description : TODO
 *******************************************************/

#ifndef _VIDEO_FILE_DATA_PRODUCER_H
#define _VIDEO_FILE_DATA_PRODUCER_H

#include <fstream>

#include "experiment_data.h"
#include "uv_timer.h"

namespace bifrost {
typedef enum {
  NALU_TYPE_SLICE = 1,
  NALU_TYPE_DPA = 2,
  NALU_TYPE_DPB = 3,
  NALU_TYPE_DPC = 4,
  NALU_TYPE_IDR = 5,
  NALU_TYPE_SEI = 6,
  NALU_TYPE_SPS = 7,
  NALU_TYPE_PPS = 8,
  NALU_TYPE_AUD = 9,
  NALU_TYPE_EOSEQ = 10,
  NALU_TYPE_EOSTREAM = 11,
  NALU_TYPE_FILL = 12,
} NaluType;

typedef enum {
  NALU_PRIORITY_DISPOSABLE = 0,
  NALU_PRIRITY_LOW = 1,
  NALU_PRIORITY_HIGH = 2,
  NALU_PRIORITY_HIGHEST = 3
} NaluPriority;

class H264FileDataProducer : public ExperimentDataProducerInterface,
                             public UvTimer::Listener {
 public:
  explicit H264FileDataProducer(uint32_t ssrc, uv_loop_t *loop);
  ~H264FileDataProducer();

 public:
  void OnTimer(UvTimer *timer);

  RtpPacketPtr CreateData() override;

 private:
  NaluType PrintfH264Frame(int j, int nLen, int nFrameType);
  int GetH264FrameLen(int nPos, size_t nTotalSize, uint8_t *data);
  bool ReadOneH264Frame();
  void H264FrameAnnexbSplitNalu(
      uint8_t *data, uint32_t len,
      std::function<bool(uint8_t *data, uint32_t len, uint32_t start_code_len)> on_nalu);
  void ReadWebRTCRtpPacketizer();
  void GetRtpExtensions(RtpPacketPtr& packet);

 private:
  std::ifstream h264_data_file_;
  uint8_t *buffer_ = nullptr;
  size_t size_ = 0;
  int frame_count_ = 0;  // 帧数
  int read_offset = 0;   //偏移量
  uint8_t *payload_;
  size_t frame_len_;

  std::vector<RtpPacketPtr> rtp_packet_vec_;

  UvTimer *read_frame_timer_;

  uint32_t ssrc_;
  uint16_t sequence_{ 0u };
};
}  // namespace bifrost

#endif  //_VIDEO_FILE_DATA_PRODUCER_H

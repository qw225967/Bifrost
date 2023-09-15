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

namespace bifrost {
class H264FileDataProducer : public ExperimentDataProducerInterface {
 public:
  explicit H264FileDataProducer(uint32_t ssrc);
  ~H264FileDataProducer();

  RtpPacketPtr CreateData() override;
 private:
  void PrintfH264Frame(int j, int nLen, int nFrameType);
  int GetH264FrameLen(int nPos, size_t nTotalSize, uint8_t *data);
  void ReadOneH264Frame();


 private:
  std::ifstream h264_data_file_;
  uint8_t *buffer_ = nullptr;
  size_t size_ = 0;
  int frame_count_ = 0;  // 帧数
  int read_offset = 0;  //偏移量

  std::vector<RtpPacketPtr> rtp_packet_vec_;
};
} // namespace bifrost

#endif  //_VIDEO_FILE_DATA_PRODUCER_H

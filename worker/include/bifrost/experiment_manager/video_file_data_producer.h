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
class VideoFileDataProducer : public ExperimentDataProducerInterface {
 public:
  VideoFileDataProducer();
  ~VideoFileDataProducer();

  RtpPacketPtr CreateData() override;

 private:
  std::ifstream data_file_;

};
} // namespace bifrost

#endif  //_VIDEO_FILE_DATA_PRODUCER_H

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/26 11:22 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_data.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_EXPERIMENT_DATA_H
#define WORKER_EXPERIMENT_DATA_H

#include <iostream>

#include "common.h"
#include "rtp_packet.h"

namespace bifrost {
struct ExperimentGccData {
  ExperimentGccData(uint32_t available_bitrate, uint32_t sent_bitrate,
                    std::vector<double> trend)
      : AvailableBitrate(available_bitrate),
        SentBitrate(sent_bitrate),
        Trend(trend) {}
  uint32_t AvailableBitrate;
  uint32_t SentBitrate;
  std::vector<double> Trend;
};

class ExperimentDataProducerInterface {
 public:
  virtual RtpPacketPtr CreateData() = 0;
};
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_DATA_H

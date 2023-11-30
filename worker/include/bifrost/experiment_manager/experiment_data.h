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
struct ExperimentDumpData {
  ExperimentDumpData(uint32_t available_bitrate, uint32_t sent_bitrate,
                     std::vector<double> trends)
      : AvailableBitrate(available_bitrate),
        SentBitrate(sent_bitrate),
        Trends(trends),
        Jitter(0),
        FractionLost(0),
        PacketsLost(0),
        Rtt(0) {}

  ExperimentDumpData(uint32_t jitter, uint8_t fraction_lost,
                     int32_t packets_lost, float rtt)
      : Jitter(jitter),
        FractionLost(fraction_lost),
        PacketsLost(packets_lost),
        Rtt(rtt) {}

  uint32_t Jitter;
  uint8_t FractionLost;
  int32_t PacketsLost;
  float Rtt;

  uint32_t AvailableBitrate;
  uint32_t SentBitrate;
  std::vector<double> Trends;
};

class ExperimentDataProducerInterface {
 public:
  virtual RtpPacketPtr CreateData() = 0;
};
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_DATA_H

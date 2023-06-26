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

namespace bifrost {
struct ExperimentGccData {
  ExperimentGccData(uint32_t available_bitrate, uint32_t sent_bitrate)
      : AvailableBitrate(available_bitrate), SentBitrate(sent_bitrate) {}
  uint32_t AvailableBitrate;
  uint32_t SentBitrate;
};
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_DATA_H

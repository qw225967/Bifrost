/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/15 4:46 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_manager.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_EXPERIMENT_MANAGER_H
#define WORKER_EXPERIMENT_MANAGER_H

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "experiment_data.h"

namespace bifrost {
class ExperimentManager {
 public:
  struct GccExperimentConfig {
    // GCC拥塞检测计算趋势的窗口值。
    // 例如：值为20，那么接收到feedback的时候，收到20个delta就会计算一次斜率。
    uint32_t TrendLineWindowSize;

    // GCC拥塞检测使用delta的斜率检测拥塞的阈值。
    float TrendLineThreshold;
  };

 public:
  ExperimentManager();
  ~ExperimentManager();

  void AddTransportNumber(uint8_t number) {
    trend_map_[number] = {};
    ExperimentGccData temp(0, 0, 0);
    bitrate_map_[number] = temp;
  }

  void InitTransportColumn() {
    this->gcc_data_file_ << "TimeStamp";
    this->gcc_trend_data_file_ << "TimeStamp";
    for (int i = 0; i < trend_map_.size(); i++) {
      this->gcc_data_file_ << ",AvailableBitrate" << i << ",";
      this->gcc_data_file_ << "SentBitrate" << i;
      this->gcc_trend_data_file_ << ",Trend" << i;
    }
    this->gcc_data_file_ << std::endl;
    this->gcc_trend_data_file_ << std::endl;
  }

  void DumpGccDataToCsv(uint8_t number, uint32_t count, uint32_t sample_size,
                        ExperimentGccData data);

 private:
  // gcc experiment
  std::ofstream gcc_data_file_;
  std::ofstream gcc_trend_data_file_;
  // transport
  std::unordered_map<uint8_t, ExperimentGccData> bitrate_map_;
  std::unordered_map<uint8_t, std::vector<ExperimentGccData>> trend_map_;
  std::unordered_map<uint8_t, bool> bitrate_count_flag_map_;
  std::unordered_map<uint8_t, bool> trend_count_flag_map_;
  // mutex
  std::mutex locker;
};
typedef std::shared_ptr<ExperimentManager> ExperimentManagerPtr;
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_MANAGER_H

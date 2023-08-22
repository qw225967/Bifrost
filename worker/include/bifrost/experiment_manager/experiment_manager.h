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
#include <memory>

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

  void PostDataToShow();

 private:
  // 估计码率展示和发送码率展示
  std::ofstream gcc_data_file_;
  // gcc趋势展示
  std::ofstream gcc_trend_data_file_;

  // mutex
  std::mutex locker;
};
typedef std::shared_ptr<ExperimentManager> ExperimentManagerPtr;
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_MANAGER_H

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
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "experiment_data.h"
#include "uv_loop.h"
#include "uv_timer.h"

namespace bifrost {
class ExperimentManager : public UvTimer::Listener {
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

  void RunDumpData() { this->uv_loop_->RunLoop(); }

  // UvTimer
  void OnTimer(UvTimer *timer);

  void PostGccDataToShow(uint8_t number, ExperimentDumpData &data);
  void PostRRDataToShow(uint8_t number, ExperimentDumpData &data);

 private:
  void BitrateCalculationDump(
      size_t &max_trend_size, std::string &now_str,
      std::unordered_map<uint8_t, ExperimentDumpData> tmp_map);
  void TrendLineCalculationDump(
      size_t max_trend_size, std::string &now_str,
      std::unordered_map<uint8_t, ExperimentDumpData> tmp_map);
  void ReceiverReportDump(std::string &now_str);
  std::vector<double> ComplementTrendVecWithSize(size_t max_size,
                                                 std::vector<double> trends);

 private:
  // uv 自己一个定时落地能力
  UvLoop *uv_loop_;
  UvTimer *dump_data_timer_;

  // 每个周期落地的数据记录
  std::unordered_map<uint8_t, ExperimentDumpData> dump_data_map_;
  std::string current_cycle_time_str_;
  uint8_t cycle_trend_ms_fraction_;

  // 估计码率展示和发送码率展示
  std::ofstream gcc_data_file_;
  // gcc趋势展示
  std::ofstream gcc_trend_data_file_;
  // RR信息展示
  std::ofstream rr_data_file_;

  // mutex
  std::mutex locker;
};
typedef std::shared_ptr<ExperimentManager> ExperimentManagerPtr;
}  // namespace bifrost

#endif  // WORKER_EXPERIMENT_MANAGER_H

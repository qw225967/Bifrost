/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/15 4:47 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_manager.cpp
 * @description : TODO
 *******************************************************/

#include "bifrost/experiment_manager/experiment_manager.h"

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace bifrost {
// 默认只创建5列数据落地，也就是默认支持5个端同时竞争测试
const uint8_t DefaultCreateColumn = 5;
// 默认落地间隔300ms一次
const uint16_t DefaultDumpDataInterval = 200;

ExperimentManager::ExperimentManager()
    : cycle_trend_ms_fraction_(
          1000 /
          DefaultDumpDataInterval) /* 按秒周期计算 trend 时间戳，需要换算参数*/
{
  // 1.初始化行列名
  this->gcc_data_file_.open("../data/gcc_data_file.csv",
                            std::ios::out | std::ios::trunc);
  this->gcc_trend_data_file_.open("../data/gcc_trend_data_file.csv",
                                  std::ios::out | std::ios::trunc);
  this->rr_data_file_.open("../data/rr_data_file.csv",
                           std::ios::out | std::ios::trunc);
  this->gcc_data_file_ << "TimeStamp";
  this->gcc_trend_data_file_ << "TimeStamp";
  this->rr_data_file_ << "TimeStamp";

  for (int i = 0; i < DefaultCreateColumn; i++) {
    this->gcc_data_file_ << ",AvailableBitrate" << i << ",SentBitrate" << i;
    this->gcc_trend_data_file_ << ",Trend" << i;
    this->rr_data_file_ << ",Jitter" << i << ",FractionLost" << i
                        << ",PacketsLost" << i << ",Rtt" << i;
  }
  this->gcc_data_file_ << std::endl;
  this->gcc_trend_data_file_ << std::endl;
  this->rr_data_file_ << std::endl;

  // 2.创建uv loop
  this->uv_loop_ = new UvLoop;
  this->uv_loop_->ClassInit();

  // 3.创建落地定时器
  this->dump_data_timer_ = new UvTimer(this, this->uv_loop_->get_loop().get());
  this->dump_data_timer_->Start(DefaultDumpDataInterval,
                                DefaultDumpDataInterval);
}
ExperimentManager::~ExperimentManager() {
  delete uv_loop_;
  delete this->dump_data_timer_;

  if (this->gcc_data_file_.is_open()) this->gcc_data_file_.close();
  if (this->gcc_trend_data_file_.is_open()) this->gcc_trend_data_file_.close();
  if (this->rr_data_file_.is_open()) this->rr_data_file_.close();
}

void ExperimentManager::PostGccDataToShow(uint8_t number,
                                          ExperimentDumpData &data) {
  locker.lock();
  auto ite = dump_data_map_.find(number);
  if (ite != dump_data_map_.end()) {
    auto temp = ite->second;
    dump_data_map_.erase(ite);
    temp.Trends = data.Trends;
    temp.SentBitrate = data.SentBitrate;
    temp.AvailableBitrate = data.AvailableBitrate;
    dump_data_map_.insert(std::make_pair(number, temp));
  } else {
    dump_data_map_.insert(std::make_pair(number, data));
  }
  locker.unlock();
}

void ExperimentManager::PostRRDataToShow(uint8_t number,
                                         ExperimentDumpData &data) {
  locker.lock();
  auto ite = dump_data_map_.find(number);
  if (ite != dump_data_map_.end()) {
    auto temp = ite->second;
    dump_data_map_.erase(ite);
    temp.FractionLost = data.FractionLost;
    temp.Jitter = data.Jitter;
    temp.PacketsLost = data.PacketsLost;
    temp.Rtt = data.Rtt;
    dump_data_map_.insert(std::make_pair(number, temp));
  } else {
    dump_data_map_.insert(std::make_pair(number, data));
  }
  locker.unlock();
}

// 自我产生平均数据，补足不满最大size的数据
std::vector<double> ExperimentManager::ComplementTrendVecWithSize(
    size_t max_size, std::vector<double> trends) {
  std::vector<double> result;

  size_t current_size = trends.size();
  if (current_size == 0 || current_size == max_size) return trends;

  /*  1.可整除，
   *    max = 6， size = 2 ——> 0,x,x,3,x,x
   *    max = 6， size = 3 ——> 0,x,2,x,4,x
   *
   *  2.不可整除，
   *    max = 7， size = 2 ——> 0,x,x,3,x,x + x
   *    max = 7,  size = 3 ——> 0,x,2,x,4,x + x
   *
   *  由上可知，
   *    需要先确定是否能整除 max % size == 0 ?
   *    能：则使用 max / size 作为间隔增加数
   *    不能：则在上述基础上增加余数
   *
   *  3.大于一半，
   *    max = 7， size = 4 ——> 0,x,2,x,4,x,6
   *    max = 7， size = 5 ——> 0,x,2,x,4,5,6
   *    并且，当size > max / 2，不需要添加后数
   */

  size_t internal_count = max_size / current_size;  // 间隔增加数
  size_t add_count = max_size - current_size;       // 模拟增加的数
  double pre_trend = trends[0];
  result.push_back(pre_trend);

  for (int i = 1; i < current_size; i++) {
    double tmp_trend = (pre_trend + trends[i]) / 2;
    for (int j = 0; j < internal_count; j++) {
      if (add_count <= 0) {
        break;
      }
      result.push_back(tmp_trend);
      add_count--;
    }
    result.push_back(trends[i]);
  }
  return result;
}

void ExperimentManager::BitrateCalculationDump(
    size_t &max_trend_size, std::string &now_str,
    std::unordered_map<uint8_t, ExperimentDumpData> tmp_map) {
  // 4.落地码率相关
  char temp_str[4096];
  auto tmp_time_interval =
      int(1000 / DefaultDumpDataInterval - cycle_trend_ms_fraction_) *
      DefaultDumpDataInterval;

  tmp_time_interval =
      tmp_time_interval >= 1000 ? 999 : tmp_time_interval;  // 避免超过 1s 统计

  sprintf(temp_str, "%03d", tmp_time_interval);
  this->gcc_data_file_ << now_str << "." << temp_str;
  for (int i = 0; i < DefaultCreateColumn; i++) {
    uint32_t available_rate = 0;
    uint32_t send_rate = 0;
    auto ite = tmp_map.find(i);
    if (ite != tmp_map.end()) {
      available_rate = ite->second.AvailableBitrate;
      send_rate = ite->second.SentBitrate;

      // 统计最大trend的size，用于对齐落地数据行
      max_trend_size = max_trend_size > ite->second.Trends.size()
                           ? max_trend_size
                           : ite->second.Trends.size();
    }
    this->gcc_data_file_ << "," << available_rate << "," << send_rate;
  }
  this->gcc_data_file_ << std::endl;
}

void ExperimentManager::TrendLineCalculationDump(
    size_t max_trend_size, std::string &now_str,
    std::unordered_map<uint8_t, ExperimentDumpData> tmp_map) {
  // 5.落地趋势线行数同步
  for (int i = 0; i < DefaultCreateColumn; i++) {
    auto ite = tmp_map.find(i);
    if (ite != tmp_map.end()) {
      // 同步所有行
      auto temp_trends =
          this->ComplementTrendVecWithSize(max_trend_size, ite->second.Trends);
      ite->second.Trends = temp_trends;
    }
  }

  // 6.落地趋势线
  for (int i = 0; i < max_trend_size; i++) {
    double trend = 0;
    // 换算毫秒
    auto now_str_ms = now_str;
    char temp_str[4096];

    uint16_t minuend = 0;
    if (max_trend_size > 1)
      minuend = i * (DefaultDumpDataInterval / (max_trend_size - 1));

    auto ms = int((1000 / DefaultDumpDataInterval - cycle_trend_ms_fraction_) *
                      DefaultDumpDataInterval +
                  minuend);
    if (ms == 1000) continue;
    sprintf(temp_str, "%03d", ms);
    now_str_ms = now_str_ms + "." + temp_str;
    this->gcc_trend_data_file_ << now_str_ms;

    // 每列都添加
    for (int j = 0; j < DefaultCreateColumn; j++) {
      auto ite = tmp_map.find(j);
      if (ite != tmp_map.end()) {
        // 同步所有行
        if (!ite->second.Trends.empty()) trend = ite->second.Trends[i];
      }
      this->gcc_trend_data_file_ << "," << trend;
      trend = 0;
    }
    this->gcc_trend_data_file_ << std::endl;
  }
  cycle_trend_ms_fraction_--;
}

void ExperimentManager::ReceiverReportDump(std::string &now_str) {
  // 当前行全是0则不落地
  bool flag = false;
  for (int i = 0; i < DefaultCreateColumn; i++) {
    auto ite = dump_data_map_.find(i);
    if (ite != dump_data_map_.end() &&
        (ite->second.Rtt != 0.0 || ite->second.Jitter != 0 ||
         ite->second.FractionLost != 0 || ite->second.PacketsLost != 0)) {
      flag = true;
    }
  }

  if (!flag) return;

  this->rr_data_file_ << now_str << ".000";
  for (int i = 0; i < DefaultCreateColumn; i++) {
    ExperimentDumpData temp_data(0, 0, 0, 0.0);
    auto ite = dump_data_map_.find(i);
    if (ite != dump_data_map_.end()) {
      temp_data = ite->second;
      dump_data_map_.erase(ite);
    }
    this->rr_data_file_ << "," << temp_data.Jitter << ","
                        << (uint32_t)temp_data.FractionLost << ","
                        << temp_data.PacketsLost << "," << temp_data.Rtt;
  }
  this->rr_data_file_ << std::endl;
}

void ExperimentManager::OnTimer(UvTimer *timer) {
  if (timer == this->dump_data_timer_) {
    // 1.加锁拷贝一份，不多做循环加锁
    locker.lock();
    auto tmp_map = dump_data_map_;
    locker.unlock();

    // 2.上个周期没更新则返回
    if (tmp_map.empty()) return;

    // 3.获取当前时间
    auto now_str = String::get_now_str_s();
    if (current_cycle_time_str_ != now_str) {
      current_cycle_time_str_ = now_str;
      cycle_trend_ms_fraction_ = 1000 / DefaultDumpDataInterval;
    }

    size_t max_trend_size = 0;

    // 4.开始统计
    // 4.1 码率统计
    this->BitrateCalculationDump(max_trend_size, now_str, tmp_map);
    // 4.2 趋势线统计
    this->TrendLineCalculationDump(max_trend_size, now_str, tmp_map);
    // 4.3 接收者报告统计
    this->ReceiverReportDump(now_str);
  }
}

}  // namespace bifrost
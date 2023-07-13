/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/15 4:47 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_manager.cpp
 * @description : TODO
 *******************************************************/

#include "experiment_manager.h"

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace bifrost {
ExperimentManager::ExperimentManager() {
  this->gcc_data_file_.open("../data/gcc_data_file.csv",
                            std::ios::out | std::ios::trunc);
  this->gcc_trend_data_file_.open("../data/gcc_trend_data_file.csv",
                                  std::ios::out | std::ios::trunc);
}
ExperimentManager::~ExperimentManager() {
  if (this->gcc_data_file_.is_open()) this->gcc_data_file_.close();
  if (this->gcc_trend_data_file_.is_open()) this->gcc_trend_data_file_.close();
}

void ExperimentManager::DumpGccDataToCsv(uint8_t number, uint32_t count,
                                         uint32_t sample_size,
                                         ExperimentGccData data) {
  time_t t = time(nullptr);
  struct tm* now = localtime(&t);

  std::stringstream timeStr;
  char temp_str[1024];

  // 以下依次把年月日的数据加入到字符串中
  timeStr << now->tm_year + 1900 << "-";
  memset(temp_str, 0, 1024);
  sprintf(temp_str, "%02d", now->tm_mon + 1);
  timeStr << temp_str << "-";
  memset(temp_str, 0, 1024);
  sprintf(temp_str, "%02d", now->tm_mday);
  timeStr << temp_str << "T";
  memset(temp_str, 0, 1024);
  sprintf(temp_str, "%02d", now->tm_hour);
  timeStr << temp_str << ":";
  memset(temp_str, 0, 1024);
  sprintf(temp_str, "%02d", now->tm_min);
  timeStr << temp_str << ":";
  memset(temp_str, 0, 1024);
  sprintf(temp_str, "%02d", now->tm_sec);
  timeStr << temp_str << ".";
  memset(temp_str, 0, 1024);

  locker.lock();
  if (data.Trend == 0 && data.SentBitrate != 0 && data.AvailableBitrate != 0) {
    bitrate_map_[number] = data;
    bitrate_count_flag_map_[number] = true;
  }

  bool is_need_bitrate_dump = true;
  if (bitrate_count_flag_map_.empty()) is_need_bitrate_dump = false;
  for (auto flag : bitrate_count_flag_map_) {
    is_need_bitrate_dump = is_need_bitrate_dump && flag.second;
  }

  if (is_need_bitrate_dump) {
    sprintf(temp_str, "%03d", 0);
    timeStr << temp_str;
    this->gcc_data_file_ << timeStr.str();
    for (uint8_t i = 0; i < bitrate_map_.size(); i++) {
      this->gcc_data_file_ << "," << bitrate_map_[i].AvailableBitrate;
      this->gcc_data_file_ << "," << bitrate_map_[i].SentBitrate;
      bitrate_count_flag_map_[i] = false;
    }
    this->gcc_data_file_ << std::endl;
  }

  if (data.Trend != 0) {
    trend_map_[number].push_back(data);
    if (count == sample_size) trend_count_flag_map_[number] = true;
  }

  bool is_need_trend_dump = true;
  if (trend_count_flag_map_.empty()) is_need_trend_dump = false;
  for (auto flag : trend_count_flag_map_) {
    is_need_trend_dump = is_need_trend_dump && flag.second;
  }

  if (is_need_trend_dump) {
    uint8_t max_size = 0;
    for (auto& trends : trend_map_) {
      max_size = max_size == 0 ? uint8_t(trends.second.size())
                               : uint8_t(max_size > trends.second.size()
                                             ? max_size
                                             : trends.second.size());
    }
    double last_trend;

    for (uint8_t i = 0; i < max_size; i++) {
      std::stringstream temp;
      temp << timeStr.str();
      memset(temp_str, 0, 1024);
      sprintf(temp_str, "%03d",
              (1000 * (i)) / max_size);  // 平均换算1s内的采样点
      temp << temp_str;
      this->gcc_trend_data_file_ << temp.str();
      for (auto& trends : trend_map_) {
        if (trends.second.size() < max_size) {
          if (i >= trends.second.size()) {
            this->gcc_trend_data_file_ << ","
                                       << std::setiosflags(std::ios::fixed)
                                       << std::setprecision(8) << last_trend;
          } else {
            last_trend = trends.second[i].Trend;
            this->gcc_trend_data_file_
                << "," << std::setiosflags(std::ios::fixed)
                << std::setprecision(8) << trends.second[i].Trend;
          }
        } else {
          this->gcc_trend_data_file_ << "," << std::setiosflags(std::ios::fixed)
                                     << std::setprecision(8)
                                     << trends.second[i].Trend;
        }
      }
      this->gcc_trend_data_file_ << std::endl;
    }
    for (uint8_t i = 0; i < trend_count_flag_map_.size(); i++) {
      trend_count_flag_map_[i] = false;
      trend_map_[i].clear();
    }
  }
  locker.unlock();
}
}  // namespace bifrost
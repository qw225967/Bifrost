/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/15 4:47 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_manager.cpp
 * @description : TODO
 *******************************************************/

#include "experiment_manager.h"

#include <ctime>
#include <sstream>

namespace bifrost {
ExperimentManager::ExperimentManager() {
  this->gcc_data_file_.open("../data/gcc_data_file.csv",
                            std::ios::out | std::ios::trunc);
  this->gcc_data_file_ << "TimeStamp,"
                       << "AvailableBitrate,"
                       << "SentBitrate" << std::endl;
}
ExperimentManager::~ExperimentManager() { this->gcc_data_file_.close(); }

void ExperimentManager::DumpGccDataToCsv(ExperimentGccData data) {
  gcc_data_vec_.push_back(data);

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
  sprintf(temp_str, "%02d", now->tm_min);
  timeStr << temp_str << ":";
  sprintf(temp_str, "%02d", now->tm_sec);
  timeStr << temp_str;

  this->gcc_data_file_ << timeStr.str() << "," << data.AvailableBitrate << ","
                       << data.SentBitrate << std::endl;
}
}  // namespace bifrost
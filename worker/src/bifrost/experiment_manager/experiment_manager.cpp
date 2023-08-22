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

}
}  // namespace bifrost
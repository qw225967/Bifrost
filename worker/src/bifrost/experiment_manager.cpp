/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/15 4:47 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : experiment_manager.cpp
 * @description : TODO
 *******************************************************/

#include "experiment_manager.h"

namespace bifrost {
ExperimentManager::ExperimentManager() {
  this->gcc_data_file_.open("../data/gcc_data_file.csv",
                            std::ios::out | std::ios::trunc);
}
ExperimentManager::~ExperimentManager() { this->gcc_data_file_.close(); }

void ExperimentManager::DumpGccDataToCsv(ExperimentGccData data) {
  gcc_data_vec_.push_back(data);
  this->gcc_data_file_ << data.AvailableBitrate
                       << "," << data.SentBitrate
                       << std::endl;
}
}  // namespace bifrost
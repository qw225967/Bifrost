/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 10:28 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : main.cpp
 * @description : TODO
 *******************************************************/

#include <memory>
#include <string>
#include <thread>

#include "bifrost/experiment_manager/experiment_manager.h"
#include "quiche/quic/core/quic_types.h"
#include "setting.h"
#include "transport.h"

void ThreadRunExperimentDataDump(
    std::shared_ptr<bifrost::ExperimentManager> &ptr) {
  std::cout << "ThreadRunExperimentDataDump" << std::endl;
  ptr->RunDumpData();
}

int main() {
  // 读取配置文件
  std::string config_path(PUBLISHER_CONFIG_FILE_PATH_STRING);
  bifrost::Settings::AnalysisConfigurationFile(config_path);

  bifrost::ExperimentManagerPtr ptr =
      std::make_shared<bifrost::ExperimentManager>();
  std::thread experiment_runner(ThreadRunExperimentDataDump, ref(ptr));

  auto temp0 = std::make_shared<bifrost::Transport>(
      bifrost::Transport::SinglePublish, 0, ptr,
      quic::CongestionControlType::kBBR);  // number 为传输标号，从 0 开始

  temp0->Run();

  return 0;
}
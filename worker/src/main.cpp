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

#include "experiment_manager.h"
#include "quiche/quic/core/quic_types.h"
#include "setting.h"
#include "transport.h"

void ThreadPublish0(std::shared_ptr<bifrost::Transport> &ptr) {
  std::cout << "ThreadPublish0 run" << std::endl;
  ptr->Run();
}

void ThreadPublish1(std::shared_ptr<bifrost::Transport> &ptr,
                    bifrost::ExperimentManagerPtr &experiment) {
  //  experiment->StartZeroDump(1);
  //  sleep(30);
  //  std::cout << "ThreadPublish1 run" << std::endl;
  //  experiment->StopZeroDump(1);
  ptr->Run();
}

int main() {
  // 读取配置文件
  std::string config_path(PUBLISHER_CONFIG_FILE_PATH_STRING);
  bifrost::Settings::AnalysisConfigurationFile(config_path);

  bifrost::ExperimentManagerPtr ptr =
      std::make_shared<bifrost::ExperimentManager>();

  auto temp0 = std::make_shared<bifrost::Transport>(
      bifrost::Transport::SinglePublish, 0, ptr,
      quic::CongestionControlType::kBBR);
  ptr->AddTransportNumber(0);
//  auto temp1 = std::make_shared<bifrost::Transport>(
//      bifrost::Transport::SinglePublish, 1, ptr,
//      quic::CongestionControlType::kGoogCC);
//  ptr->AddTransportNumber(1);
  ptr->InitTransportColumn();

  std::thread publish0(ThreadPublish0, ref(temp0));
//  std::thread publish1(ThreadPublish1, ref(temp1), ref(ptr));

  publish0.join();
//  publish1.join();

  return 0;
}
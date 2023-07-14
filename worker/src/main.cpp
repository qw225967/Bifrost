/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 10:28 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : main.cpp
 * @description : TODO
 *******************************************************/

#include <string>
#include <thread>
#include <memory>

#include "setting.h"
#include "transport.h"
#include "experiment_manager.h"

void ThreadPublish(std::shared_ptr<bifrost::Transport> &ptr) {
  ptr->Run();
}

int main() {
  // 读取配置文件
  std::string config_path(PUBLISHER_CONFIG_FILE_PATH_STRING);
  bifrost::Settings::AnalysisConfigurationFile(config_path);

  bifrost::ExperimentManagerPtr ptr = std::make_shared<bifrost::ExperimentManager>();


  auto temp0 =
      std::make_shared<bifrost::Transport>(bifrost::Transport::SinglePublish, 0, ptr);
  ptr->AddTransportNumber(0);
  auto temp1 =
      std::make_shared<bifrost::Transport>(bifrost::Transport::SinglePublish, 1, ptr);
  ptr->AddTransportNumber(1);
  ptr->InitTransportColumn();

  std::thread publish0(ThreadPublish, ref(temp0));
  std::thread publish1(ThreadPublish, ref(temp1));

  publish0.join();
  publish1.join();

  return 0;
}
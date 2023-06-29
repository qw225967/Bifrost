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

#include "setting.h"
#include "transport.h"

int main() {
  // 读取配置文件
  std::string config_path(PUBLISHER_CONFIG_FILE_PATH_STRING);
  bifrost::Settings::AnalysisConfigurationFile(config_path);

  auto temp =
      std::make_shared<bifrost::Transport>(bifrost::Transport::SinglePublish);
  temp->Run();

  return 0;
}
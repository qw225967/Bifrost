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
  bifrost::Settings::AnalysisConfigurationFile(CONFIG_FILE_PATH_STRING);

  bifrost::Transport t(
      bifrost::Settings::server_configuration_,
      bifrost::Settings::client_configuration_map_["remote_client_1"]);

  t.Run();

  return 0;
}
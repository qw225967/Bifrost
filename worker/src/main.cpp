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

void tread_run() {
  bifrost::Transport t(
      bifrost::Settings::client_configuration_map_["remote_client_1"],
      bifrost::Settings::server_configuration_);

  auto temp = std::make_shared<bifrost::Transport>(t);
  t.SetRemoteTransport(bifrost::Settings::server_configuration_.ssrc, temp);

  t.RunDataProducer();
  t.Run();
}
int main() {
  // 读取配置文件
  bifrost::Settings::AnalysisConfigurationFile(CONFIG_FILE_PATH_STRING);

  std::thread td(tread_run);

  bifrost::Transport t(
      bifrost::Settings::server_configuration_,
      bifrost::Settings::client_configuration_map_["remote_client_1"]);

  auto temp = std::make_shared<bifrost::Transport>(t);
  t.SetRemoteTransport(
      bifrost::Settings::client_configuration_map_["remote_client_1"].ssrc,
      temp);

  t.Run();

  return 0;
}
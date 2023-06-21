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

void ThreadPlay() {
  auto temp =
      std::make_shared<bifrost::Transport>(bifrost::Transport::SinglePlay);
  temp->Run();
}

int main() {
  // 读取配置文件
  std::string publish_config_path(PUBLISHER_CONFIG_FILE_PATH_STRING);
  std::string play_config_path(PLAYER_CONFIG_FILE_PATH_STRING);
  bifrost::Settings::AnalysisConfigurationFile(publish_config_path,
                                               play_config_path);

  std::thread play(ThreadPlay);

  auto temp =
      std::make_shared<bifrost::Transport>(bifrost::Transport::SinglePublish);
  temp->Run();

  return 0;
}
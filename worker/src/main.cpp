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
#include "uv_loop.h"

int main() {
  // 初始化 libuv loop
  bifrost::UvLoop loop;
  loop.ClassInit();

  // 读取配置文件
  bifrost::Settings::AnalysisConfigurationFile(CONFIG_FILE_PATH_STRING);

  // 启动事件循环
  loop.RunLoop();

  return 0;
}
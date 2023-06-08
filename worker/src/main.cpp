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

#include "server_router.h"
#include "uv_loop.h"

int main() {
  bifrost::UvLoop::ClassInit();

  bifrost::ServerRouter::GetServerRouter();

  bifrost::UvLoop::PrintVersion();
  bifrost::UvLoop::RunLoop();

  return 0;
}
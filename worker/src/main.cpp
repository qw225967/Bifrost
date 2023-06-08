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

#include "udp_router.h"
#include "uv_loop.h"

void testTreadFunc() {
  uv_udp_t udp_send_socket;
  struct sockaddr_in recv_addr, send_addr;
  auto loop = uv_default_loop();
  uv_udp_init(loop, &udp_send_socket);
  uv_ip4_addr("127.0.0.1", 8666, &send_addr);
  uv_udp_connect(&udp_send_socket, (const struct sockaddr *)&send_addr);
  uv_udp_send_t send_req;
  uv_buf_t read_buf;
  std::string aa("1234");
  read_buf.base = const_cast<char *>(aa.c_str());
  read_buf.len = sizeof(aa.c_str());
  send_req.data = (void *)&read_buf;
  while (1) {
    uv_udp_send(&send_req, &udp_send_socket, &read_buf, 1, NULL, NULL);
    uv_sleep(200);
  }
}

int main() {
  bifrost::UvLoop::ClassInit();

  bifrost::UvLoop::PrintVersion();
  bifrost::UvLoop::RunLoop();

  return 0;
}
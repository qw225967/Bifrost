/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 4:06 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : port_manager.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_PORT_MANAGER_H
#define WORKER_PORT_MANAGER_H

#include <uv.h>
#include <string>

namespace bifrost {
class PortManager {
 private:
  enum class Transport : uint8_t { UDP = 1, TCP };

 public:
  static uv_udp_t* BindUdp() { return reinterpret_cast<uv_udp_t*>(BindPort()); }
  static uv_tcp_t* BindTcp(std::string& ip) {
    return reinterpret_cast<uv_tcp_t*>(Bind(Transport::TCP, ip));
  }
  static void UnbindUdp(std::string& ip, uint16_t port) {
    return Unbind(Transport::UDP, ip, port);
  }
  static void UnbindTcp(std::string& ip, uint16_t port) {
    return Unbind(Transport::TCP, ip, port);
  }

 private:
  static uv_handle_t* Bind(Transport transport, std::string& ip);
  static void Unbind(Transport transport, std::string& ip, uint16_t port);
  static std::vector<bool>& GetPorts(Transport transport,
                                     const std::string& ip);
  static uv_handle_t* BindPort();
};
}  // namespace bifrost

#endif  // WORKER_PORT_MANAGER_H

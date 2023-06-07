/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 4:07 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : port_manager.cpp
 * @description : TODO
 *******************************************************/

#include "port_manager.h"
#include "setting.h"
#include "uv_loop.h"
#include <iostream>

static inline void onClose(uv_handle_t* handle) { delete handle; }

namespace bifrost {
/* Class methods. */
uv_handle_t* PortManager::BindPort() {
  std::cout << "[port manager] BindPort befor "
            <<  Settings::configuration.rtcIp.c_str() << std::endl;
  // First normalize the IP. This may throw if invalid IP.
  Utils::IP::NormalizeIp(Settings::configuration.rtcIp);
  std::cout << "[port manager] BindPort after "
            << Settings::configuration.rtcIp.c_str() << std::endl;

  struct sockaddr_storage
      bindAddr;  // NOLINT(cppcoreguidelines-pro-type-member-init)
  uv_handle_t* uvHandle{nullptr};
  int err;
  int flags{0};
  int family = Utils::IP::GetFamily(Settings::configuration.rtcIp);
  switch (family) {
    case AF_INET: {
      err = uv_ip4_addr(Settings::configuration.rtcIp.c_str(), 0,
                        reinterpret_cast<struct sockaddr_in*>(&bindAddr));
      std::cout << "[port manager] uv_ip4_addr" << std::endl;
      if (err != 0)
        std::cout << "[port manager] uv_ip4_addr() failed: "
                  << uv_strerror(err) << std::endl;

      break;
    }

    case AF_INET6: {
      err = uv_ip6_addr(Settings::configuration.rtcIp.c_str(), 0,
                        reinterpret_cast<struct sockaddr_in6*>(&bindAddr));
      std::cout << "[port manager] uv_ip6_addr" << std::endl;
      if (err != 0)
        std::cout << "[port manager] uv_ip6_addr() failed: "
                  << uv_strerror(err) << std::endl;

      // Don't also bind into IPv4 when listening in IPv6.
      // flags |= UV_UDP_IPV6ONLY;

      break;
    }

    // This cannot happen.
    default: {
      std::cout << "[port manager] unknown IP family" << std::endl;
      std::cout << "[port manager] unknown IP family" << std::endl;
    }
  }

  // Set the chosen port into the sockaddr struct.
  switch (family) {
    case AF_INET:
      std::cout << "[port manager] unknown AF_INET" << std::endl;
      (reinterpret_cast<struct sockaddr_in*>(&bindAddr))->sin_port =
          htons(Settings::configuration.rtcPort);
      break;

    case AF_INET6:
      std::cout << "[port manager] unknown AF_INET6" << std::endl;
      (reinterpret_cast<struct sockaddr_in6*>(&bindAddr))->sin6_port =
          htons(Settings::configuration.rtcPort);
      break;
  }

  uvHandle = reinterpret_cast<uv_handle_t*>(new uv_udp_t());
  uv_udp_init_ex(UvLoop::GetLoop(), reinterpret_cast<uv_udp_t*>(uvHandle),
                 UV_UDP_RECVMMSG);
  uv_udp_bind(reinterpret_cast<uv_udp_t*>(uvHandle),
              reinterpret_cast<const struct sockaddr*>(&bindAddr), flags);
  // If it failed, close the handle and check the reason.
  // uv_close(reinterpret_cast<uv_handle_t*>(uvHandle),
  // static_cast<uv_close_cb>(onClose));
  return static_cast<uv_handle_t*>(uvHandle);
}

uv_handle_t* PortManager::Bind(Transport transport, std::string& ip) {
  // Do nothing
  return nullptr;
}

void PortManager::Unbind(Transport transport, std::string& ip, uint16_t port) {
  // Do nothing
}
}  // namespace bifrost
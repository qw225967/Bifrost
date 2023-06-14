/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 4:44 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : utils.cpp
 * @description : TODO
 *******************************************************/

#include "utils.h"

#include <uv.h>

#include <iostream>

namespace bifrost {
int IP::get_family(const std::string& ip) {
  if (ip.size() >= INET6_ADDRSTRLEN) return AF_UNSPEC;

  auto ipPtr = ip.c_str();
  char ipBuffer[INET6_ADDRSTRLEN] = {0};

  if (uv_inet_pton(AF_INET, ipPtr, ipBuffer) == 0)
    return AF_INET;
  else if (uv_inet_pton(AF_INET6, ipPtr, ipBuffer) == 0)
    return AF_INET6;
  else
    return AF_UNSPEC;
}

void IP::get_address_info(const struct sockaddr* addr, int& family,
                          std::string& ip, uint16_t& port) {
  char ipBuffer[INET6_ADDRSTRLEN] = {0};
  int err;

  switch (addr->sa_family) {
    case AF_INET: {
      err = uv_inet_ntop(
          AF_INET,
          std::addressof(
              reinterpret_cast<const struct sockaddr_in*>(addr)->sin_addr),
          ipBuffer, sizeof(ipBuffer));

      if (err)
        std::cout << "[utils] uv_inet_ntop() failed: " << uv_strerror(err)
                  << std::endl;

      port = static_cast<uint16_t>(
          ntohs(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port));

      break;
    }

    case AF_INET6: {
      err = uv_inet_ntop(
          AF_INET6,
          std::addressof(
              reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_addr),
          ipBuffer, sizeof(ipBuffer));

      if (err)
        std::cout << "[utils] uv_inet_ntop() failed: " << uv_strerror(err)
                  << std::endl;

      port = static_cast<uint16_t>(
          ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port));

      break;
    }

    default: {
      std::cout << "[utils] unknown network family: "
                << static_cast<int>(addr->sa_family) << std::endl;
    }
  }

  family = addr->sa_family;
  ip.assign(ipBuffer);
}

void IP::NormalizeIp(std::string& ip) {
  static sockaddr_storage addrStorage;
  char ipBuffer[INET6_ADDRSTRLEN] = {0};
  int err;

  switch (IP::get_family(ip)) {
    case AF_INET: {
      err = uv_ip4_addr(ip.c_str(), 0,
                        reinterpret_cast<struct sockaddr_in*>(&addrStorage));

      if (err != 0) {
        std::cout << "[utils] uv_ip4_addr() failed [ip:'" << ip.c_str()
                  << "']: " << uv_strerror(err) << std::endl;
      }

      err = uv_ip4_name(reinterpret_cast<const struct sockaddr_in*>(
                            std::addressof(addrStorage)),
                        ipBuffer, sizeof(ipBuffer));

      if (err != 0) {
        std::cout << "[utils] uv_ipv4_name() failed [ip:'" << ip.c_str()
                  << "']: %s" << uv_strerror(err) << std::endl;
      }

      ip.assign(ipBuffer);

      break;
    }

    case AF_INET6: {
      err = uv_ip6_addr(ip.c_str(), 0,
                        reinterpret_cast<struct sockaddr_in6*>(&addrStorage));

      if (err != 0) {
        std::cout << "[utils] uv_ip6_addr() failed [ip:'" << ip.c_str()
                  << "']: %s" << uv_strerror(err) << std::endl;
      }

      err = uv_ip6_name(reinterpret_cast<const struct sockaddr_in6*>(
                            std::addressof(addrStorage)),
                        ipBuffer, sizeof(ipBuffer));

      if (err != 0) {
        std::cout << "[utils] uv_ipv6_name() failed [ip:'" << ip.c_str()
                  << "']: %s" << uv_strerror(err) << std::endl;
      }

      ip.assign(ipBuffer);

      break;
    }

    default: {
      std::cout << "[utils] invalid IP '" << ip.c_str() << "'" << std::endl;
    }
  }
}
}  // namespace bifrost
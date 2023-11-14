/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/14 1:47 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : uv_run.h
 * @description : TODO
 *******************************************************/

#ifndef _UV_RUN_H
#define _UV_RUN_H

#include <uv.h>

#include <iostream>

namespace ns3proxy {
class UvRun {
 public:
  static int get_family(const std::string& ip) {
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
  
  static void get_address_info(const struct sockaddr* addr, int& family,
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
          std::cout << "[proxy] uv_inet_ntop() failed: " << uv_strerror(err)
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
          std::cout << "[proxy] uv_inet_ntop() failed: " << uv_strerror(err)
          << std::endl;

        port = static_cast<uint16_t>(
            ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port));

        break;
      }

      default: {
        std::cout << "[proxy] unknown network family: "
        << static_cast<int>(addr->sa_family) << std::endl;
      }
    }

    family = addr->sa_family;
    ip.assign(ipBuffer);
  }

  static void NormalizeIp(std::string& ip) {
    static sockaddr_storage addrStorage;
    char ipBuffer[INET6_ADDRSTRLEN] = {0};
    int err;

    switch (get_family(ip)) {
      case AF_INET: {
        err = uv_ip4_addr(ip.c_str(), 0,
                          reinterpret_cast<struct sockaddr_in*>(&addrStorage));

        if (err != 0) {
          std::cout << "[proxy] uv_ip4_addr() failed [ip:'" << ip.c_str()
                    << "']: " << uv_strerror(err) << std::endl;
        }

        err = uv_ip4_name(reinterpret_cast<const struct sockaddr_in*>(
                              std::addressof(addrStorage)),
                          ipBuffer, sizeof(ipBuffer));

        if (err != 0) {
          std::cout << "[proxy] uv_ipv4_name() failed [ip:'" << ip.c_str()
                    << "']: %s" << uv_strerror(err) << std::endl;
        }

        ip.assign(ipBuffer);

        break;
      }

      case AF_INET6: {
        err = uv_ip6_addr(ip.c_str(), 0,
                          reinterpret_cast<struct sockaddr_in6*>(&addrStorage));

        if (err != 0) {
          std::cout << "[proxy] uv_ip6_addr() failed [ip:'" << ip.c_str()
                    << "']: %s" << uv_strerror(err) << std::endl;
        }

        err = uv_ip6_name(reinterpret_cast<const struct sockaddr_in6*>(
                              std::addressof(addrStorage)),
                          ipBuffer, sizeof(ipBuffer));

        if (err != 0) {
          std::cout << "[proxy] uv_ipv6_name() failed [ip:'" << ip.c_str()
                    << "']: %s" << uv_strerror(err) << std::endl;
        }

        ip.assign(ipBuffer);

        break;
      }

      default: {
        std::cout << "[proxy] invalid IP '" << ip.c_str() << "'" << std::endl;
      }
    }
  }

  static uv_handle_t* BindPort(std::string ip, uint16_t port, uv_loop_t* loop) {
    std::cout << "[proxy] BindPort by config befor " << ip.c_str() << std::endl;
    // First normalize the IP. This may throw if invalid IP.
    NormalizeIp(ip);
    std::cout << "[proxy] BindPort by config after " << ip.c_str() << std::endl;

    struct sockaddr_storage
        bind_addr;  // NOLINT(cppcoreguidelines-pro-type-member-init)
    uv_handle_t* uvHandle{nullptr};
    int err;
    int flags{0};
    int family = get_family(ip);
    switch (family) {
      case AF_INET: {
        err = uv_ip4_addr(ip.c_str(), 0,
                          reinterpret_cast<struct sockaddr_in*>(&bind_addr));
        std::cout << "[proxy] uv_ip4_addr" << std::endl;
        if (err != 0)
          std::cout << "[proxy] uv_ip4_addr() failed: " << uv_strerror(err)
                    << std::endl;

        break;
      }

      case AF_INET6: {
        err = uv_ip6_addr(ip.c_str(), 0,
                          reinterpret_cast<struct sockaddr_in6*>(&bind_addr));
        std::cout << "[proxy] uv_ip6_addr" << std::endl;
        if (err != 0)
          std::cout << "[proxy] uv_ip6_addr() failed: " << uv_strerror(err)
                    << std::endl;

        // Don't also bind into IPv4 when listening in IPv6.
        // flags |= UV_UDP_IPV6ONLY;

        break;
      }

      // This cannot happen.
      default: {
        std::cout << "[proxy] unknown IP family" << std::endl;
        std::cout << "[proxy] unknown IP family" << std::endl;
      }
    }

    // Set the chosen port into the sockaddr struct.
    switch (family) {
      case AF_INET:
        std::cout << "[proxy] unknown AF_INET" << std::endl;
        (reinterpret_cast<struct sockaddr_in*>(&bind_addr))->sin_port =
            htons(port);
        break;

      case AF_INET6:
        std::cout << "[proxy] unknown AF_INET6" << std::endl;
        (reinterpret_cast<struct sockaddr_in6*>(&bind_addr))->sin6_port =
            htons(port);
        break;
    }

    uvHandle = reinterpret_cast<uv_handle_t*>(new uv_udp_t());
    uv_udp_init_ex(loop, reinterpret_cast<uv_udp_t*>(uvHandle),
                   UV_UDP_RECVMMSG);
    uv_udp_bind(reinterpret_cast<uv_udp_t*>(uvHandle),
                reinterpret_cast<const struct sockaddr*>(&bind_addr), flags);

    return static_cast<uv_handle_t*>(uvHandle);
  }
};
}  // namespace ns3proxy

#endif  //_UV_RUN_H

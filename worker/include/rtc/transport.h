/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:09 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : transport.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_TRANSPORT_H
#define WORKER_TRANSPORT_H

#include "udp_router.h"

namespace bifrost {
class Transport {
 public:
  Transport(UdpRouter* router, sockaddr* addr) {}
  ~Transport() {}

 private:
  UdpRouter* udp_router_;
  sockaddr* send_addr_;
};
}  // namespace bifrost

#endif  // WORKER_TRANSPORT_H

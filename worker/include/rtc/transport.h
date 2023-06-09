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
#include "uv_loop.h"

namespace bifrost {
typedef std::shared_ptr<UvLoop> UvLoopPtr;
typedef std::shared_ptr<UdpRouter> UdpRouterPtr;

class Transport : UdpRouter::UdpRouterObServer {
 public:
  Transport(Settings::Configuration config);
  ~Transport();

  void RunLoop() { this->uv_loop_->RunLoop(); }

 private:
  UvLoopPtr uv_loop_;
  UdpRouterPtr udp_router_;
};
}  // namespace bifrost

#endif  // WORKER_TRANSPORT_H

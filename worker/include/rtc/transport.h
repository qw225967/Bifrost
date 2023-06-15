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
#include "data_producer.h"
#include "uv_timer.h"

namespace bifrost {
typedef std::shared_ptr<UvTimer> UvTimerPtr;
typedef std::shared_ptr<UvLoop> UvLoopPtr;
typedef std::shared_ptr<UdpRouter> UdpRouterPtr;
typedef std::shared_ptr<sockaddr> SockAddressPtr;
typedef std::shared_ptr<DataProducer> DataProducerPtr;

class Transport : public UvTimer::Listener, public UdpRouter::UdpRouterObServer {
 public:
  Transport(Settings::Configuration &local_config,
            Settings::Configuration &remote_config);
  ~Transport();

 public:
  // UvTimer
  void OnTimer(UvTimer* timer) override;

  // UdpRouterObServer
  void OnUdpRouterPacketReceived(
      bifrost::UdpRouter* socket, const uint8_t* data, size_t len,
      const struct sockaddr* remoteAddr) override {}

  void Run();
 private:
  // send packet producer
  DataProducerPtr data_producer_;

  // uv
  UvTimerPtr producer_timer;
  UvLoopPtr uv_loop_;
  UdpRouterPtr udp_router_;
  SockAddressPtr udp_remote_address_;
};
}  // namespace bifrost

#endif  // WORKER_TRANSPORT_H

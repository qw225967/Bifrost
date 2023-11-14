/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/13 4:22 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : ns3_proxy.h
 * @description : TODO
 *******************************************************/

#ifndef _NS3_PROXY_H
#define _NS3_PROXY_H

#include <uv.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "io/udp_socket.h"
#include "io/uv_run.h"

namespace ns3proxy {
class Ns3Proxy {
 public:
  Ns3Proxy();
  ~Ns3Proxy();

 public:
  // 内部函数
  uint32_t GetSsrcByData(const uint8_t* data, size_t len);
  bool IsRtp(const uint8_t* data, size_t len);
  bool IsRtcp(const uint8_t* data, size_t len);
};

class ProxyIn : public UdpSocket, public Ns3Proxy {
 public:
  class ProxyInObserver {
   public:
    virtual void ProxyInReceivePacket(uint32_t ssrc, const uint8_t* data,
                                      size_t len,
                                      const struct sockaddr* addr) = 0;
  };

 public:
  ProxyIn(std::string& ip, uint16_t port,
          ProxyInObserver *observer, uv_loop_t* loop)
      : observer_(observer),
        UdpSocket(
            reinterpret_cast<uv_udp_t*>(UvRun::BindPort(ip, port, loop))) {}

  ~ProxyIn() {}

 private:
  // 继承UdpSocket
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;

 private:
  ProxyInObserver* observer_;
};

class ProxyOut : public UdpSocket, public Ns3Proxy {
 public:
  class ProxyOutObserver {
   public:
    virtual void ProxyOutReceivePacket(uint32_t ssrc, const uint8_t* data,
                                       size_t len,
                                       const struct sockaddr* addr) = 0;
  };

 public:
  ProxyOut(std::string& ip, uint16_t port,
           ProxyOutObserver *observer, uv_loop_t* loop)
      : observer_(observer),
        UdpSocket(
            reinterpret_cast<uv_udp_t*>(UvRun::BindPort(ip, port, loop))) {}

  ~ProxyOut() {}

 private:
  // 继承UdpSocket
  void UserOnUdpDatagramReceived(const uint8_t* data, size_t len,
                                 const struct sockaddr* addr) override;

 private:
  ProxyOutObserver* observer_;
};

class ProxyManager : public ProxyIn::ProxyInObserver,
                     public ProxyOut::ProxyOutObserver {
 public:
  ProxyManager();
  ~ProxyManager();

 public:
  void RunProxy() { uv_run(this->loop_, UV_RUN_DEFAULT); }

 private:
  void ProxyInReceivePacket(uint32_t ssrc, const uint8_t* data, size_t len,
                            const struct sockaddr* addr) override;
  void ProxyOutReceivePacket(uint32_t ssrc, const uint8_t* data, size_t len,
                             const struct sockaddr* addr) override;

 private:
  // 代理方向的ip、port
  sockaddr* proxy_addr_;
  std::unordered_map<uint32_t, const struct sockaddr*> ssrc_remote_map_;
  std::mutex locker;
  std::shared_ptr<ProxyIn> proxy_in_;
  std::shared_ptr<ProxyOut> proxy_out_;
  uv_loop_t * loop_;
};
}  // namespace ns3proxy

#endif  //_NS3_PROXY_H

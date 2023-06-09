/*******************************************************
 * @author      : dog head
 * @date        : Created in 2022/10/19 2:42 下午
 * @mail        : qw225967@github.com
 * @project     : Bifrost
 * @file        : io_udp.h
 * @description : TODO
 *******************************************************/

#ifndef BIFROST_IO_UDP_H
#define BIFROST_IO_UDP_H

#include <uv.h>

#include <memory>

namespace bifrost {
typedef std::shared_ptr<uv_loop_t> UvLoopTPtr;
class UvLoop {
 public:
  void ClassInit();
  void ClassDestroy();
  void PrintVersion();
  void RunLoop();

 public:
  UvLoopTPtr get_loop() { return this->loop_; }
  uint32_t get_time_s() {
    return static_cast<uint32_t>(uv_hrtime() / 1000000000u);
  }
  uint64_t get_time_ms() {
    return static_cast<uint64_t>(uv_hrtime() / 1000000u);
  }
  uint64_t get_time_us() { return static_cast<uint64_t>(uv_hrtime() / 1000u); }
  uint64_t get_time_ns() { return uv_hrtime(); }
  // Used within libwebrtc dependency which uses int64_t values for time
  // representation.
  int64_t get_time_ms_int64() {
    return static_cast<int64_t>(UvLoop::get_time_ms());
  }
  // Used within libwebrtc dependency which uses int64_t values for time
  // representation.
  int64_t get_time_us_int64() {
    return static_cast<int64_t>(UvLoop::get_time_us());
  }

 private:
  UvLoopTPtr loop_;
};
}  // namespace bifrost

#endif  // BIFROST_IO_UDP_H

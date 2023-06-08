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

namespace bifrost {
class UvLoop {
 public:
  static void ClassInit();
  static void ClassDestroy();
  static void PrintVersion();
  static void RunLoop();
  static uv_loop_t* GetLoop() { return UvLoop::loop_; }
  static uint32_t GetTimeS() {
    return static_cast<uint32_t>(uv_hrtime() / 1000000000u);
  }
  static uint64_t GetTimeMs() {
    return static_cast<uint64_t>(uv_hrtime() / 1000000u);
  }
  static uint64_t GetTimeUs() {
    return static_cast<uint64_t>(uv_hrtime() / 1000u);
  }
  static uint64_t GetTimeNs() { return uv_hrtime(); }
  // Used within libwebrtc dependency which uses int64_t values for time
  // representation.
  static int64_t GetTimeMsInt64() {
    return static_cast<int64_t>(UvLoop::GetTimeMs());
  }
  // Used within libwebrtc dependency which uses int64_t values for time
  // representation.
  static int64_t GetTimeUsInt64() {
    return static_cast<int64_t>(UvLoop::GetTimeUs());
  }

 private:
  static uv_loop_t* loop_;
};
}  // namespace bifrost

#endif  // BIFROST_IO_UDP_H

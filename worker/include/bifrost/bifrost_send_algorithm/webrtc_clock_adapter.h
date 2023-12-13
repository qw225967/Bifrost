/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/12/11 9:44 上午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : webrtc_clock_adapter.h
 * @description : TODO
 *******************************************************/

#ifndef _WEBRTC_CLOCK_ADAPTER_H
#define _WEBRTC_CLOCK_ADAPTER_H

#include <system_wrappers/include/clock.h>

#include "uv_loop.h"

namespace bifrost {
class WebRTCClockAdapter : public webrtc::Clock {
 public:
  WebRTCClockAdapter(UvLoop *loop) : loop_(loop) {}

 public:
  webrtc::Timestamp CurrentTime() override {
    return webrtc::Timestamp::us(loop_->get_time_us_int64());
  }
  int64_t TimeInMilliseconds() override { return CurrentTime().ms(); }
  int64_t TimeInMicroseconds() override { return CurrentTime().us(); }

  // Retrieve an NTP absolute timestamp.
  webrtc::NtpTime CurrentNtpTime() override {
    int64_t now_ms = loop_->get_time_ms_int64();
    uint32_t seconds = (now_ms / 1000) + webrtc::kNtpJan1970;
    uint32_t fractions = static_cast<uint32_t>(
        (now_ms % 1000) * webrtc::kMagicNtpFractionalUnit / 1000);
    return webrtc::NtpTime(seconds, fractions);
  }

  // Retrieve an NTP absolute timestamp in milliseconds.
  int64_t CurrentNtpInMilliseconds() override {
    return loop_->get_time_ms_int64() +
           1000 * static_cast<int64_t>(webrtc::kNtpJan1970);
  }

 private:
  UvLoop* loop_;
};
}  // namespace bifrost
#endif  //_WEBRTC_CLOCK_ADAPTER_H

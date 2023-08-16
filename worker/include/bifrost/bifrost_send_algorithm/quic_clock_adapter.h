/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/25 4:12 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : quic_time_adapter.h
 * @description : TODO
 *******************************************************/

#ifndef _QUIC_TIME_ADAPTER_H
#define _QUIC_TIME_ADAPTER_H

#include "quiche/quic/core/quic_clock.h"
#include "uv_loop.h"

namespace bifrost {
class QuicClockAdapter : public quic::QuicClock {
 public:
  QuicClockAdapter(UvLoop** uv_loop) : uv_loop_(*uv_loop) {}

  quic::QuicTime ApproximateNow() const override {
    return quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(
                                        this->uv_loop_->get_time_ms_int64());
  }
  quic::QuicTime Now() const override {
    return quic::QuicTime::Zero() + quic::QuicTimeDelta::FromMilliseconds(
                                        this->uv_loop_->get_time_ms_int64());
  }
  quic::QuicWallTime WallNow() const override {
    return quic::QuicWallTime::FromUNIXMicroseconds(
        this->uv_loop_->get_time_us_int64());
  }

 private:
  // uv
  UvLoop* uv_loop_;
};

}  // namespace bifrost

#endif  //_QUIC_TIME_ADAPTER_H

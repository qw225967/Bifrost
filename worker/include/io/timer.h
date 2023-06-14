/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 1:55 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : timer.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_TIMER_H
#define WORKER_TIMER_H

#include <uv.h>

#include "common.h"

namespace bifrost {
class UvTimer {
 public:
  class Listener {
   public:
    virtual ~Listener() = default;

   public:
    virtual void OnTimer(UvTimer* timer) = 0;
  };

 public:
  explicit UvTimer(Listener* listener, uv_loop_t* loop);
  UvTimer& operator=(const UvTimer&) = delete;
  UvTimer(const UvTimer&) = delete;
  ~UvTimer();

 public:
  void Close();
  void Start(uint64_t timeout, uint64_t repeat = 0);
  void Stop();
  void Reset();
  void Restart();
  uint64_t GetTimeout() const { return this->timeout; }
  uint64_t GetRepeat() const { return this->repeat; }
  bool IsActive() const {
    return uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0;
  }

  /* Callbacks fired by UV events. */
 public:
  void OnUvTimer();

 private:
  // Passed by argument.
  Listener* listener{nullptr};
  // Allocated by this.
  uv_timer_t* uvHandle{nullptr};
  // Others.
  bool closed{false};
  uint64_t timeout{0u};
  uint64_t repeat{0u};
};
}  // namespace bifrost

#endif  // WORKER_TIMER_H

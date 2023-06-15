/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 1:56 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : timer.cpp
 * @description : TODO
 *******************************************************/

#include "uv_timer.h"

#include <iostream>

#include "uv_loop.h"

namespace bifrost {
/* Static methods for UV callbacks. */
inline static void onTimer(uv_timer_t* handle) {
  static_cast<UvTimer*>(handle->data)->OnUvTimer();
}

inline static void onClose(uv_handle_t* handle) { delete handle; }

/* Instance methods. */
UvTimer::UvTimer(Listener* listener, uv_loop_t* loop) : listener(listener) {
  this->uvHandle = new uv_timer_t;
  this->uvHandle->data = static_cast<void*>(this);

  int err = uv_timer_init(loop, this->uvHandle);

  if (err != 0) {
    delete this->uvHandle;
    this->uvHandle = nullptr;
  }
}

UvTimer::~UvTimer() {
  if (!this->closed) Close();
}

void UvTimer::Close() {
  if (this->closed) return;

  this->closed = true;

  uv_close(reinterpret_cast<uv_handle_t*>(this->uvHandle),
           static_cast<uv_close_cb>(onClose));
}

void UvTimer::Start(uint64_t timeout, uint64_t repeat) {
  if (this->closed) return;

  this->timeout = timeout;
  this->repeat = repeat;

  if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0) Stop();

  int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer),
                           timeout, repeat);

  if (err != 0) std::cout << "[timer] uv timer start err" << std::endl;
}

void UvTimer::Stop() {
  if (this->closed) return;

  int err = uv_timer_stop(this->uvHandle);

  if (err != 0) std::cout << "[timer] uv timer stop err" << std::endl;
}

void UvTimer::Reset() {
  if (this->closed) return;

  if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) == 0) return;

  if (this->repeat == 0u) return;

  int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer),
                           this->repeat, this->repeat);

  if (err != 0) std::cout << "[timer] uv timer reset start err" << std::endl;
}

void UvTimer::Restart() {
  if (this->closed) return;

  if (uv_is_active(reinterpret_cast<uv_handle_t*>(this->uvHandle)) != 0) Stop();

  int err = uv_timer_start(this->uvHandle, static_cast<uv_timer_cb>(onTimer),
                           this->timeout, this->repeat);

  if (err != 0) std::cout << "[timer] uv timer restart err" << std::endl;
}

inline void UvTimer::OnUvTimer() {
  // Notify the listener.
  this->listener->OnTimer(this);
}
}  // namespace bifrost
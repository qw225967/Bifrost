/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 10:53 上午
 * @mail        : qw225967@github.com
 * @project     : Bifrost
 * @file        : uv_loop.cpp
 * @description : TODO
 *******************************************************/

#include "uv_loop.h"

#include <cstdlib>  // std::abort()
#include <iostream>

namespace bifrost {
/* Static variables. */
uv_loop_t* UvLoop::loop_{nullptr};

/* Static methods. */
void UvLoop::ClassInit() {
  // NOTE: Logger depends on this so we cannot log anything here.

  UvLoop::loop_ = new uv_loop_t;

  int err = uv_loop_init(UvLoop::loop_);

  if (err != 0) std::cout << "[uv loop] initialization failed" << std::endl;
}

void UvLoop::ClassDestroy() {
  // This should never happen.
  if (UvLoop::loop_ != nullptr) {
    uv_loop_close(UvLoop::loop_);
    delete UvLoop::loop_;
  }
}

void UvLoop::PrintVersion() {
  std::cout << "[uv loop] version:" << uv_version_string() << std::endl;
}

void UvLoop::RunLoop() { uv_run(UvLoop::loop_, UV_RUN_DEFAULT); }
}  // namespace bifrost
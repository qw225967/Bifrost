/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 10:53 上午
 * @mail        : qw225967@github.com
 * @project     : Bifrost
 * @file        : uv_loop.cpp
 * @description : TODO
 *******************************************************/

#include "uv_loop.h"

#include <iostream>

namespace bifrost {
/* Static methods. */
void UvLoop::ClassInit() {
  // NOTE: Logger depends on this so we cannot log anything here.

  this->loop_ = std::make_shared<uv_loop_t>();

  int err = uv_loop_init(this->loop_.get());

  if (err != 0) std::cout << "[uv loop] initialization failed" << std::endl;
}

void UvLoop::ClassDestroy() {
  // This should never happen.
  if (this->loop_ != nullptr) {
    uv_loop_close(this->loop_.get());
    this->loop_.reset();
  }
}

void UvLoop::PrintVersion() {
  std::cout << "[uv loop] version:" << uv_version_string() << std::endl;
}

void UvLoop::RunLoop() { uv_run(this->loop_.get(), UV_RUN_DEFAULT); }
}  // namespace bifrost
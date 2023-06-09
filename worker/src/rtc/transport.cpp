/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/8 5:09 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : transport.cpp
 * @description : TODO
 *******************************************************/

#include "transport.h"

namespace bifrost {
Transport::Transport(Settings::Configuration config) {
  this->uv_loop_ = std::make_shared<UvLoop>();

  // 1.init loop
  this->uv_loop_->ClassInit();

  // 2.get config
  //   2.1 don't use default model : just get config default
#ifdef USING_DEFAULT_AF_CONFIG
  this->udp_router_ =
      std::make_shared<UdpRouter>(this->uv_loop_->get_loop().get());

//   2.2 use default model : get config by json file
#elif
  this->udp_router_ =
      std::make_shared<UdpRouter>(config, this->uv_loop_->get_loop().get());
#endif
}
}  // namespace bifrost
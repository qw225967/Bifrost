/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/20 9:55 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : player.cpp
 * @description : TODO
 *******************************************************/

#include "player.h"

namespace bifrost {
Player::Player(const struct sockaddr* remote_addr, UvLoop** uv_loop,
               Observer* observer)
    : uv_loop_(*uv_loop), observer_(observer) {
  // 1.remote address set
  this->udp_remote_address_ = std::make_shared<sockaddr>(*remote_addr);

  // 2.tcc server
  this->tcc_server_ = std::make_shared<TransportCongestionControlServer>(
      this, MtuSize, &this->uv_loop_);
}
}  // namespace bifrost
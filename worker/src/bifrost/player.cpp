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
Player::Player(Settings::Configuration& remote_config, UvLoop** uv_loop,
               Observer* observer)
    : remote_(remote_config), uv_loop_(*uv_loop), observer_(observer) {
  // 1.remote address set
  auto remote_addr = Settings::get_sockaddr_by_config(remote_config);
  this->udp_remote_address_ = std::make_shared<sockaddr>(remote_addr);

  // 2.tcc server
  this->tcc_server_ = std::make_shared<TransportCongestionControlServer>(
      this, MtuSize, &this->uv_loop_);
}
}  // namespace bifrost
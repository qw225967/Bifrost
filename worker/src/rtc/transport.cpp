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
Transport::Transport(Settings::Configuration &local_config,
                     Settings::Configuration &remote_config) {
  this->uv_loop_ = std::make_shared<UvLoop>();

  // 1.init loop
  this->uv_loop_->ClassInit();

  // 2.get config
  //   2.1 don't use default model : just get config default
#ifdef USING_DEFAULT_AF_CONFIG
  this->udp_router_ =
      std::make_shared<UdpRouter>(this->uv_loop_->get_loop().get());

  //   2.2 use default model : get config by json file
#else
  this->udp_router_ = std::make_shared<UdpRouter>(
      local_config, this->uv_loop_->get_loop().get());

  // 3.set remote address
  auto remote_addr = Settings::get_sockaddr_by_config(remote_config);
  this->udp_remote_address_ = std::make_shared<sockaddr>(remote_addr);
#endif

  // 4.create data producer
  this->data_producer_ = std::make_shared<DataProducer>(this->uv_loop_->get_loop().get());
}

Transport::~Transport() {
  if (this->uv_loop_ != nullptr) this->uv_loop_.reset();

  if (this->udp_router_ != nullptr) this->udp_router_.reset();
}

void Transport::Run() {
  this->data_producer_->RangeCreateData();
  this->uv_loop_->RunLoop();
}

}  // namespace bifrost
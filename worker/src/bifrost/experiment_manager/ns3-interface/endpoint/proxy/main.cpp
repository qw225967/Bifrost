/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/14 2:18 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : main.cpp
 * @description : TODO
 *******************************************************/

#include <thread>
#include <memory>
#include "ns3_proxy.h"


int main() {

  std::shared_ptr<ns3proxy::ProxyManager> manager = std::make_shared<ns3proxy::ProxyManager>();

  manager->RunProxy()

  return 0;
}
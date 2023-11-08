/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/10/27 2:38 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : BifrostTapNetDeviceHelper.cpp
 * @description : TODO
 *******************************************************/

#include "BifrostTapNetDeviceHelper.h"

#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>
#include <csignal>

namespace ns3 {
void onSignal(int signum) {
  std::cout << "Received signal: " << signum << std::endl;
  // see https://gitlab.com/nsnam/ns-3-dev/issues/102
  Simulator::Stop();
  Simulator::Destroy();
  NS_FATAL_ERROR(signum);
}

BifrostTapNetDeviceHelper::BifrostTapNetDeviceHelper(BifrostDevice dev_left,
                                                     BifrostDevice dev_right) {
  // 1.开启实时仿真
  GlobalValue::Bind("SimulatorImplementationType",
                    StringValue("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

  // 2.创建节点并安装默认协议栈
  NodeContainer nodes;
  nodes.Create(2);
  InternetStackHelper internet;
  internet.Install(nodes);

  this->left_node_ = nodes.Get(0);
  this->right_node_ = nodes.Get(1);

  // 3.安装左节点
  this->InstallTapFdNetDevice(
      this->left_node_, dev_left.dev_name_.c_str(),
      this->get_mac_address(dev_left.dev_name_.c_str()),
      Ipv4InterfaceAddress(dev_left.ipv4_.c_str(), dev_left.v4_mask_.c_str()),
      Ipv6InterfaceAddress(dev_left.ipv6_.c_str(), 64));
  // 4.安装右节点
  this->InstallTapFdNetDevice(
      this->right_node_, dev_right.dev_name_.c_str(),
      this->get_mac_address(dev_right.dev_name_.c_str()),
      Ipv4InterfaceAddress(dev_right.ipv4_.c_str(), dev_right.v4_mask_.c_str()),
      Ipv6InterfaceAddress(dev_right.ipv6_.c_str(), 64));
}

Mac48Address BifrostTapNetDeviceHelper::get_mac_address(std::string i_face) {
  unsigned char buf[6];
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, i_face.c_str(), IFNAMSIZ - 1);
  ioctl(fd, SIOCGIFHWADDR, &ifr);
  for (unsigned int i = 0; i < 6; i++) {
    buf[i] = ifr.ifr_hwaddr.sa_data[i];
  }
  ioctl(fd, SIOCGIFMTU, &ifr);
  close(fd);
  Mac48Address mac;
  mac.CopyFrom(buf);
  return mac;
}

void BifrostTapNetDeviceHelper::InstallTapFdNetDevice(
    Ptr<Node> node, std::string device_name, Mac48AddressValue mac_address,
    Ipv4InterfaceAddress ipv4_address, Ipv6InterfaceAddress ipv6_address) const {
  // 1.创建Fd设备
  EmuFdNetDeviceHelper emu;
  emu.SetDeviceName(device_name);

  NetDeviceContainer devices = emu.Install(node);
  Ptr<NetDevice> device = devices.Get(0);
  device->SetAttribute("Address", mac_address);

  // 2.设置设备与Ipv4层交互接口
  Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
  uint32_t interface = ipv4->AddInterface(device);
  ipv4->AddAddress(interface, ipv4_address);
  ipv4->SetMetric(interface, 1);
  ipv4->SetUp(interface);

  // 3.设置设备与Ipv6层交互接口
//  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
//  ipv6->SetAttribute("IpForward", BooleanValue(true));
//  interface = ipv6->AddInterface(device);
//  ipv6->AddAddress(interface, ipv6_address);
//  ipv6->SetMetric(interface, 1);
//  ipv6->SetUp(interface);
}

void BifrostTapNetDeviceHelper::Run(Time duration) {
  // 1.设置信号截获
  signal(SIGTERM, onSignal);
  signal(SIGINT, onSignal);
  signal(SIGKILL, onSignal);

  // 2.开启全局路由
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // 3.开启Ipv6路由
  this->MassageIpv6Routing(left_node_, right_node_);
  this->MassageIpv6Routing(right_node_, left_node_);

  // 4.记录路由信息到本地文件
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(
      "dynamic-global-routing.routes", std::ios::out);
  Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(0.), routingStream);
  Ipv6RoutingHelper::PrintRoutingTableAllAt(Seconds(0.), routingStream);

  Simulator::Stop(duration);
  Simulator::Run();
  Simulator::Destroy();
}

void BifrostTapNetDeviceHelper::MassageIpv6Routing(Ptr<Node> local,
                                                   Ptr<Node> peer) {
//  Ptr<Ipv6StaticRouting> routing =
//      Ipv6RoutingHelper::GetRouting<Ipv6StaticRouting>(
//          local->GetObject<Ipv6>()->GetRoutingProtocol());
//  Ptr<Ipv6> peer_ipv6 = peer->GetObject<Ipv6>();
//  Ipv6Address dst;
//  for (uint32_t i = 0; i < peer_ipv6->GetNInterfaces(); i++)
//    for (uint32_t j = 0; j < peer_ipv6->GetNAddresses(i); j++)
//      if (peer_ipv6->GetAddress(i, j).GetAddress().CombinePrefix(64) ==
//          "fd00:cafe:cafe:50::") {
//        dst = peer_ipv6->GetAddress(i, j).GetAddress();
//        goto done;
//      }
//
//done:
//  if (dst.IsInitialized()) routing->SetDefaultRoute(dst, 2);
}
}  // namespace ns3

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/10/27 2:37 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : BifrostClientHelper.h
 * @description : TODO
 *******************************************************/

//                 +-----------+
//                 |           |
// left_node ------|  ns3-mesh |------ right_node_
//                 |           |
//                 +-----------+

#ifndef _BIFROSTCLIENTHELPER_H
#define _BIFROSTCLIENTHELPER_H

#include <string>

#include "ns3/core-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/internet-module.h"

namespace ns3 {
class BifrostTapNetDeviceHelper {
 public:
  struct BifrostDevice {
    BifrostDevice(std::string dev_name, std::string ipv4, std::string v4_mask,
                  std::string ipv6)
        : dev_name_(dev_name), ipv4_(ipv4), v4_mask_(v4_mask), ipv6_(ipv6) {}
    std::string dev_name_;
    std::string ipv4_;
    std::string v4_mask_;
    std::string ipv6_;
  };

 public:
  BifrostTapNetDeviceHelper(BifrostDevice dev_left, BifrostDevice dev_right);

  void Run(Time, bool);

  Ptr<Node> get_left() { return left_node_; }
  Ptr<Node> get_right() { return right_node_; }

 private:
  Mac48Address get_mac_address(std::string i_face);
  void MassageIpv6Routing(Ptr<Node> local, Ptr<Node> peer);
  void InstallTapFdNetDevice(Ptr<Node> node, std::string device_name,
                             Mac48AddressValue mac_address,
                             Ipv4InterfaceAddress ipv4_address,
                             Ipv6InterfaceAddress ipv6_address) const;

 private:
  Ptr<Node> left_node_, right_node_;
};
}  // namespace ns3

#endif  //_BIFROSTCLIENTHELPER_H


/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/10/27 3:15 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : BifrostPing.cpp
 * @description : TODO
 *******************************************************/

#include "../helper/BifrostTapNetDeviceHelper.h"
#include "../TopologyInterface/TopologyInterface.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3 simulator");

int main(int argc, char *argv[]) {
  BifrostTapNetDeviceHelper::BifrostDevice dev1(
      "eth0", "10.0.0.2", "255.255.0.0", "fd00:cafe:cafe:0::10");
  BifrostTapNetDeviceHelper::BifrostDevice dev2(
      "eth1", "10.100.0.2", "255.255.0.0", "fd00:cafe:cafe:100::10");

  BifrostTapNetDeviceHelper helper(dev1, dev2);

  TopologyInterface *tif = new DefaultPointToPoint;
  tif->Install(helper.get_left(), helper.get_right());

  helper.Run(Seconds(3600));
  delete tif;

  return 0;
}

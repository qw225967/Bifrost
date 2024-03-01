/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/10/27 3:15 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : BifrostPing.cpp
 * @description : TODO
 *******************************************************/

#include "../helper/BifrostTapNetDeviceHelper.h"
#include "../TopologyInterface/TopologySimpleLTE.h"
#include "../TopologyInterface/TopologyBlackHole.h"
#include "../TopologyInterface/TopologyWifiModule.h"
#include "../TopologyInterface/TopologyBurstUserLTE.h"
#include "../TopologyInterface/FileRecordRedeploy.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ns3 simulator");

int main(int argc, char *argv[]) {
  BifrostTapNetDeviceHelper::BifrostDevice dev1(
      "eth0", "10.10.0.2", "255.255.0.0", "fd00:cafe:cafe:0::10");
  BifrostTapNetDeviceHelper::BifrostDevice dev2(
      "eth1", "10.100.0.2", "255.255.0.0", "fd00:cafe:cafe:100::10");

  BifrostTapNetDeviceHelper helper(dev1, dev2);

  TopologyInterface *tif = new FileRecordRedeploy;
  tif->Install(helper.get_left(), helper.get_right());

  bool useGlobalRouting = true; // TopologySimpleLTE, FileRecordRedeploy 必须关闭全局路由，使用内部路由

  helper.Run(Seconds(36000), useGlobalRouting);
  delete tif;

  return 0;
}

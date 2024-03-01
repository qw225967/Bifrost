/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/1/5 2:40 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : TopologyBlackhole.cpp
 * @description : TODO
 *******************************************************/

#include "TopologyBlackHole.h"

#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"

namespace ns3 {

void enable(Ptr<BlackholeErrorModel> em, const Time next,
                               const int repeat) {
  static int counter = 0;
  counter++;
  std::cout << Simulator::Now().GetSeconds() << "s: Enabling blackhole"
            << std::endl;
  em->Enable();
  if (counter < repeat) {
    Simulator::Schedule(next, &enable, em, next, repeat);
  }
}

void disable(Ptr<BlackholeErrorModel> em, const Time next,
                                const int repeat) {
  static int counter = 0;
  std::cout << Simulator::Now().GetSeconds() << "s: Disabling blackhole"
            << std::endl;
  counter++;
  em->Disable();
  if (counter < repeat) {
    Simulator::Schedule(next, &disable, em, next, repeat);
  }
}

NetDeviceContainer TopologyBlackHole::Install(Ptr<Node> a, Ptr<Node> b) {
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("20ms"));

  NetDeviceContainer devices = p2p.Install(a, b);

  // capture a pcap of all packets
  p2p.EnablePcap("trace_node_left.pcap", devices.Get(0), false, true);
  p2p.EnablePcap("trace_node_right.pcap", devices.Get(1), false, true);

  TrafficControlHelper tch;
  tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize",
                       StringValue("1000p"));
  tch.Install(devices);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.50.0", "255.255.255.0");
  ipv4.Assign(devices);

  Ipv6AddressHelper ipv6;
  ipv6.SetBase("fd00:cafe:cafe:50::", 64);
  ipv6.Assign(devices);

  Ptr<BlackholeErrorModel> em = CreateObject<BlackholeErrorModel>();
  em->Disable();

  devices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
  devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

	// 时间记录
	Time start = Time(Seconds(10));
	Time end = Time(Seconds(10)) + Time(Seconds(2));

	// 第10s开始触发开始事件，并触发10次
  Simulator::Schedule(start, &enable, em, end, 10);

	// 第12s开始触发关闭事件，并触发10次
  Simulator::Schedule(end, &disable, em, end, 10);

  return devices;
}

}  // namespace ns3
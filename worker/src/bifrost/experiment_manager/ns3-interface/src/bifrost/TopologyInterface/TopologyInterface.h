/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/7 4:47 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : TopologyInterface.h
 * @description : TODO
 *******************************************************/

#ifndef _TOPOLOGYINTERFACE_H
#define _TOPOLOGYINTERFACE_H

#include "ns3/point-to-point-module.h"

namespace ns3 {

/*
 * 该类为支持NS-3的网络拓扑接口类，默认提供了一个最简单的p2p模型。
 * 可以继承该类后，进行自定义的网络拓扑，或者根据损失模型设计自定义的损失内容。
 */

class TopologyInterface {
 public:
  virtual ~TopologyInterface() {}
  virtual NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b) = 0;
};

class DefaultPointToPoint : public TopologyInterface,
                            public PointToPointHelper {
 public:
  DefaultPointToPoint(){}
  ~DefaultPointToPoint(){}

  NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b) {
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("20ms"));

    NetDeviceContainer devices = p2p.Install(a, b);

    // capture a pcap of all packets
    p2p.EnablePcap("trace_node_left.pcap", devices.Get(0),
                   false, true);
    p2p.EnablePcap("trace_node_right.pcap", devices.Get(1),
                   false, true);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.50.0", "255.255.255.0");
    ipv4.Assign(devices);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase("fd00:cafe:cafe:50::", 64);
    ipv6.Assign(devices);

    return devices;
  }
};

}  // namespace ns3

#endif  //_TOPOLOGYINTERFACE_H


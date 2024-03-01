#include "TopologyWifiModule.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

namespace ns3 {

// 参考 ns-3.40/examples/wireless/wifi-simple-infra.cc 实现
NetDeviceContainer TopologyWifiModule::Install(Ptr<Node> a, Ptr<Node> b) {
  std::string phyMode("DsssRate1Mbps");
  double rss = -80;           // -dBm

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();;
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set("RxGain", DoubleValue(0));
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(rss));
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode",
                                StringValue(phyMode),
                                "ControlMode",
                                StringValue(phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid("wifi-default");
  // setup STA
  wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
  NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, a);
  NetDeviceContainer devices = staDevice;
  // setup AP
  wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
  NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, b);
  devices.Add(apDevice);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  NodeContainer all_node;
  all_node.Add(a);
  all_node.Add(b);
  mobility.Install(all_node);

  // InternetStackHelper internet;
  // internet.Install(all_node);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.50.0", "255.255.255.0");
  // Ipv4InterfaceContainer i = ipv4.Assign(devices);
  ipv4.Assign(devices);

  return devices;

  // 注释 TopologyWifiModule.cpp 代码
  // Ipv4InterfaceContainer i = ipv4.Assign(devices);

  // TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  // Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(0), tid);
  // InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
  // recvSink->Bind(local);
  // recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

  // Ptr<Socket> source = Socket::CreateSocket(c.Get(1), tid);
  // InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), 80);
  // source->SetAllowBroadcast(true);
  // source->Connect(remote);

  // // Tracing
  // wifiPhy.EnablePcap("wifi-simple-infra", devices);

  // // Output what we are doing
  // std::cout << "Testing " << numPackets << " packets sent with receiver rss " << rss << std::endl;

  // Simulator::ScheduleWithContext(source->GetNode()->GetId(),
  //                                 Seconds(1.0),
  //                                 &GenerateTraffic,
  //                                 source,
  //                                 packetSize,
  //                                 numPackets,
  //                                 interval);

  // Simulator::Stop(Seconds(30.0));
  // Simulator::Run();
  // Simulator::Destroy();
}

}

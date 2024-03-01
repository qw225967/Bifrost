/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/1 3:41 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : TopologyBurstUserLTE.cpp
 * @description : TODO
 *******************************************************/

#include "TopologyBurstUserLTE.h"

#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"
#include "ErrorModel/user-uplink/user-uplink-congestion.h"

namespace ns3 {
	// 开始时间
	const int StartTime = 40;

	// 持续时间
	const int WorkTime = 20;

	static int counter = 0;
	static int startTime = StartTime;

	void enable(Ptr<UserUpLinkCongestionErrorModel> em, const int repeat) {
		counter++;
		int endTime = startTime + WorkTime;
		std::cout << Simulator::Now().GetSeconds() << "s: Enabling users up link run"
		<< " start time:"<< startTime << ", end time:" << endTime << std::endl;

		// 需要绝对时间，也就是相对仿真开始的时间
		em->UserCongestionEnable(startTime, endTime);
		startTime += StartTime;

		if (counter < repeat) {
			Simulator::Schedule(Time(Seconds(StartTime)), &enable, em, repeat);
		}
	}

	TopologyBurstUserLTE::TopologyBurstUserLTE() {
		this->lte_helper_ = CreateObject<LteHelper>();
		// 设置核心网
		this->epc_helper_ = CreateObject<PointToPointEpcHelper>();
		this->lte_helper_->SetEpcHelper(this->epc_helper_);

		this->pgw_ = this->epc_helper_->GetPgwNode();
	}

	NetDeviceContainer TopologyBurstUserLTE::Install(Ptr<Node> a, Ptr<Node> b) {
		// Create the Internet
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
		p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
		p2ph.SetChannelAttribute("Delay", TimeValue(NanoSeconds(10)));
		NetDeviceContainer internetDevices = p2ph.Install(this->pgw_, b);
		Ipv4AddressHelper ipv4h;
		ipv4h.SetBase("1.0.0.0", "255.0.0.0");
		Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
		internetIpIfaces.GetAddress(0).Print(std::cout);
		std::cout << std::endl;
		internetIpIfaces.GetAddress(1).Print(std::cout);
		std::cout << std::endl;
		// interface 0 is localhost, 1 is the p2p device
		//		Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);

		auto numNodePairs        = 1;
		auto distance            = 60.0;
		auto interPacketInterval = MilliSeconds(100);

		//		NodeContainer ueNodes;
		NodeContainer enbNodes;
		enbNodes.Create(numNodePairs);
		//		ueNodes.Create(numNodePairs);

		// Install Mobility Model
		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
		for (uint16_t i = 0; i < numNodePairs; i++)
		{
			positionAlloc->Add(Vector(distance * i, 0, 0));
		}
		MobilityHelper mobility;
		mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
		mobility.SetPositionAllocator(positionAlloc);
		mobility.Install(enbNodes);
		mobility.Install(a);

		// Install LTE Devices to the nodes
		NetDeviceContainer enbLteDevs = this->lte_helper_->InstallEnbDevice(enbNodes);
		NetDeviceContainer ueLteDevs  = this->lte_helper_->InstallUeDevice(a);

		// Install the IP stack on the UEs
		InternetStackHelper internet;
		// internet.Install(ueNodes);
		Ipv4InterfaceContainer ueIpIface;
		ueIpIface = this->epc_helper_->AssignUeIpv4Address(NetDeviceContainer(ueLteDevs));
		Ptr<Ipv4> ipv4 = a->GetObject<Ipv4>();
		uint32_t interface = ipv4->GetInterfaceForDevice(ueLteDevs.Get(0));
		ueIpIface.GetAddress(0).Print(std::cout);
		std::cout << std::endl;
		std::cout << "frq test interface:" << interface << std::endl;
		// Assign IP address to UEs, and install applications
		Ptr<Node> ueNode = a;

		// Attach one UE per eNodeB
		for (uint16_t i = 0; i < numNodePairs; i++)
		{
			this->lte_helper_->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i));
			// side effect: the default EPS bearer will be activated
		}

		Ipv4StaticRoutingHelper ipv4RoutingHelper;
		// 设置路由
		{
			Ptr<Ipv4StaticRouting> pgwStaticRouting =
				ipv4RoutingHelper.GetStaticRouting(this->pgw_->GetObject<Ipv4>());
			pgwStaticRouting->AddHostRouteTo(Ipv4Address("10.100.0.100"), Ipv4Address("1.0.0.0"), 2);
			pgwStaticRouting->AddHostRouteTo(Ipv4Address("10.10.0.100"), Ipv4Address("1.0.0.0"), 1);

			Ptr<Ipv4StaticRouting> ueStaticRouting =
				ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
			ueStaticRouting->AddHostRouteTo(Ipv4Address("10.100.0.100"), Ipv4Address("7.0.0.0"), 2);

			Ptr<Ipv4StaticRouting> nodeBStaticRouting =
				ipv4RoutingHelper.GetStaticRouting(b->GetObject<Ipv4>());
			nodeBStaticRouting->AddHostRouteTo(Ipv4Address("10.10.0.100"), Ipv4Address("1.0.0.0"), 2);
		}

		this->lte_helper_->EnableTraces();

		internet.EnablePcapIpv4("lena-simple-epc-remote.pcap", b->GetId (), 1, true);
		internet.EnablePcapIpv4("lena-simple-epc-ue.pcap", ueNode->GetId (), 1, true);
		internet.EnablePcapIpv4("lena-simple-epc-pgw.pcap", this->pgw_->GetId (), 1, true);


		// 上行爆发流量打向 ue -> pgw
		Ptr<UserUpLinkCongestionErrorModel> em = CreateObject<UserUpLinkCongestionErrorModel>();
		em->Disable();
		em->SetConnectNode(ipv4RoutingHelper, a);

		// 模拟第200s开始触发开启事件，触发10次
		Simulator::Schedule(Time(Seconds(StartTime)), &enable, em, 10);

		return internetDevices;
	}

}  // namespace ns3
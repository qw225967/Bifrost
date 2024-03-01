/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/1/11 2:02 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : TopologySimpleLTE.cpp
 * @description : TODO
 *******************************************************/

#include "TopologySimpleLTE.h"
#include <iostream>

/*
 *  使用SimpleLte需要修改NS3源码：vim src/lte/model/epc-sgw-pgw-application.cc (ns3-3.29) 中 第 182 行。
 *
 *
 *	将

		NS_LOG_LOGIC ("packet addressed to UE " << ueAddr);

 *	代码改为

 		// NS_LOG_LOGIC ("packet addressed to UE " << ueAddr);
 		std::map<Ipv4Address, Ptr<UeInfo> >::iterator ite = m_ueInfoByAddrMap.find (Ipv4Address("7.0.0.2"));
        Ipv4Address enbAddr1 = ite->second->GetEnbAddr ();
        uint32_t teid1 = ite->second->Classify (packet);
        if (teid1 == 0)
        {
          NS_LOG_WARN ("no matching bearer for this packet xjtu: " << ueAddr);
        }
        else
        {
          SendToS1uSocket (packet, enbAddr1, teid1);
        }
 *
 * 	强制将转发到10.10.0.100的数据使用ue的ip(7.0.0.2)转发
 *
 * 	src/Dockerfile 脚本已完成上述操作，仅需知
 *
 */


namespace ns3
{
	TopologySimpleLTE::TopologySimpleLTE()
	{
		LogComponentEnable("EpcEnbApplication",LOG_LEVEL_ALL);
		LogComponentEnable("EpcSgwPgwApplication",LOG_LEVEL_ALL);
		LogComponentEnable("VirtualNetDevice",LOG_LEVEL_ALL);

		this->lte_helper_ = CreateObject<LteHelper>();
		// 设置核心网
		this->epc_helper_ = CreateObject<PointToPointEpcHelper>();
		this->lte_helper_->SetEpcHelper(this->epc_helper_);

		this->pgw_ = this->epc_helper_->GetPgwNode();
	}

	NetDeviceContainer TopologySimpleLTE::Install(Ptr<Node> a, Ptr<Node> b)
	{

		// Create the Internet
		PointToPointHelper p2ph;
		p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
		p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
		p2ph.SetChannelAttribute("Delay", TimeValue(MilliSeconds(10)));
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

		// 设置路由
		{
			Ipv4StaticRoutingHelper ipv4RoutingHelper;
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

		return internetDevices;
	}
} // namespace ns3

/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/1 3:31 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : UserUplinkCongestion.cpp
 * @description : TODO
 *******************************************************/

#include "user-uplink-congestion.h"

NS_OBJECT_ENSURE_REGISTERED(UserUpLinkCongestionErrorModel);

const int UserNodeNumbers = 10;
const int EachCallNumbers = 20;

UserUpLinkCongestionErrorModel::UserUpLinkCongestionErrorModel()  {

};

void UserUpLinkCongestionErrorModel::SetConnectNode(Ipv4StaticRoutingHelper ipv4RoutingHelper, Ptr<Node> a) {
	// Setup two nodes
	nodes_.Create (UserNodeNumbers);

	InternetStackHelper stack;
	stack.Install (nodes_);

	nodes_.Add(a);

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Gb/s"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (10)));

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (nodes_);

	Ipv4AddressHelper address;
	address.SetBase ("100.1.0.0", "255.255.0.0");

	interfaces_ = address.Assign (csmaDevices);

	for (uint32_t u = 0; u < nodes_.GetN (); ++u)
	{
		Ptr<Node> ueNode = nodes_.Get (u);
		// Set the default gateway for the UE
		Ptr<Ipv4StaticRouting> ue_static_routing = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
		ue_static_routing->SetDefaultRoute (Ipv4Address("10.10.0.0"), 1);
		stack.EnablePcapIpv4("lena-simple-csma.pcap", ueNode->GetId (), 1, true);
	}

	for (uint16_t i = 0; i < UserNodeNumbers; i++)
	{
		UdpClientHelper dlClient (Ipv4Address("10.100.0.100"), 2000);
		dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds (100)));
		dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
		client_apps_.Add (dlClient.Install (nodes_.Get(i)));
	}
}

TypeId UserUpLinkCongestionErrorModel::GetTypeId(void) {
	static TypeId tid = TypeId("UserUpLinkCongestionErrorModel")
		.SetParent<ErrorModel>()
		.AddConstructor<UserUpLinkCongestionErrorModel>()
		;
	return tid;
}
 
bool UserUpLinkCongestionErrorModel::DoCorrupt(Ptr<Packet> p) {
	return false;
}

void UserUpLinkCongestionErrorModel::UserCongestionEnable(int startTime, int endTime) {
	client_apps_.Start(Seconds (startTime));
	client_apps_.Stop(Seconds (endTime));
}

void UserUpLinkCongestionErrorModel::DoReset(void) { }
/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/27 2:13 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : FileRecordRecurrent.cpp
 * @description : TODO
 *******************************************************/

#include "FileRecordRedeploy.h"

#include "ns3/point-to-point-module.h"
#include "ErrorModel/mix-error/mix-error.h"

namespace ns3 {

	static uint32_t index = 0;
	void enable(Ptr<MixError> em, std::vector<std::vector<uint32_t>> vec, Ptr<Node> a) {

		std::cout << Simulator::Now().GetSeconds() << "s: Enabling mix error"
		<< ", time vec:" << vec[index][0] << std::endl;

		em->SetRecordItem(vec[index]);
		em->UserUplinkCongestion(vec[index][2], vec[index][3], a);

		index++;
		if (index > vec.size() - 1)
			return;

		Time start = Time(Seconds(vec[index][0] - vec[index-1][0]));

		Simulator::Schedule(start, &enable, em, vec, a);
	}

	FileRecordRedeploy::FileRecordRedeploy() {
		this->ReadFileToDataVec();
	}

	void FileRecordRedeploy::ReadFileToDataVec() {
		this->record_data_ = {
			{47, 0, 2, 5},
			{68, 0, 11, 3},
			{76, 0, 19, 4},
			{105, 0, 4, 3},
			{107, 0, 21, 3},
			{169, 0, 8, 3},
			{189, 0, 19, 1},
			{192, 0, 35, 7},
			{272, 0, 21, 1},
			{271, 0, 47, 1},
			{274, 0, 57, 2},
			{280, 0, 7, 7},
			{316, 0, 4, 1},
			{326, 0, 4, 1},
			{364, 0, 105, 1},
			{374, 0, 6, 4},
			{397, 0, 11, 8},
		};
	}

	NetDeviceContainer FileRecordRedeploy::Install(Ptr<Node> a, Ptr<Node> b) {
		PointToPointHelper p2p;
		p2p.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
		p2p.SetChannelAttribute("Delay", StringValue("2ms"));

		NetDeviceContainer devices = p2p.Install(a, b);

		// capture a pcap of all packets
		p2p.EnablePcap("trace_node_left.pcap", devices.Get(0), false, true);
		p2p.EnablePcap("trace_node_right.pcap", devices.Get(1), false, true);

		TrafficControlHelper tch;
		tch.SetRootQueueDisc("ns3::PfifoFastQueueDisc", "MaxSize", StringValue("1000p"));
		tch.Install(devices);

		Ipv4AddressHelper ipv4;
		ipv4.SetBase("10.1.50.0", "255.255.255.0");
		ipv4.Assign(devices);

		Ipv6AddressHelper ipv6;
		ipv6.SetBase("fd00:cafe:cafe:50::", 64);
		ipv6.Assign(devices);

		Ptr<MixError> em = CreateObject<MixError>();
		em->CreatUpLinkUser(a);

		devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

		Time start = Time(Seconds(this->record_data_[index][0]));
		// 第10s开始触发开始事件，并触发10次
		Simulator::Schedule(start, &enable, em, this->record_data_, a);

		return devices;
	}
}
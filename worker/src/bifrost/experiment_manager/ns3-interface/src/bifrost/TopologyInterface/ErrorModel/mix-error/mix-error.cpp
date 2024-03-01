/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/28 10:19 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : MixError.cpp
 * @description : TODO
 *******************************************************/

#include "mix-error.h"

TypeId MixError::GetTypeId(void) {
	static TypeId tid = TypeId("MixError")
		.SetParent<ErrorModel>()
		.AddConstructor<MixError>()
		;
	return tid;
}

MixError::MixError() {}

void MixError::CreatUpLinkUser(Ptr<Node> a){

}

void MixError::SetRecordItem(std::vector<uint32_t> item) {
	this->item_ = item;
	this->enable_ = true;
}

bool MixError::DoCorrupt (Ptr<Packet> p) {
	if (!enable_) return false;

	if (this->lost_count_  < this->item_[1]) {
		this->lost_count_ ++;
		return true;
	}
	this->lost_count_ = 0;
	this->enable_ = false;
	return false;
}

void MixError::UserUplinkCongestion(uint32_t numbers, uint32_t stop_time, Ptr<Node> a) {

	ApplicationContainer client_apps;

	UdpClientHelper dlClient (Ipv4Address("10.100.0.100"), 2000);
	dlClient.SetAttribute ("Interval", TimeValue (NanoSeconds (1000000/numbers)));
	dlClient.SetAttribute ("MaxPackets", UintegerValue (1000000));
	client_apps.Add (dlClient.Install (a));

	std::cout << "nodes number:" << numbers << ", stop_time" << stop_time << std::endl;

	client_apps.Start(Seconds (0));
	client_apps.Stop(Seconds (stop_time));
}
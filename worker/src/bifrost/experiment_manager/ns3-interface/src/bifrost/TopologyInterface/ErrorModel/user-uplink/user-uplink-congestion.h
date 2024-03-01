/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/1 3:31 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : UserUplinkCongestion.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_USERUPLINKCONGESTION_H
#define WORKER_USERUPLINKCONGESTION_H

#include "ns3/error-model.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

// The BlackholeErrorModel drops all packets.
class UserUpLinkCongestionErrorModel : public ErrorModel {
public:
	UserUpLinkCongestionErrorModel();
	static TypeId GetTypeId(void);

	void SetConnectNode(Ipv4StaticRoutingHelper ipv4RoutingHelper, Ptr<Node> a);

	void UserCongestionEnable(int startTime, int endTime);

private:
	bool DoCorrupt (Ptr<Packet> p);
	void DoReset(void);

	NodeContainer nodes_;
	ApplicationContainer client_apps_;
	Ipv4InterfaceContainer interfaces_;
	int run_count_ = 0;
};


#endif // WORKER_USERUPLINKCONGESTION_H

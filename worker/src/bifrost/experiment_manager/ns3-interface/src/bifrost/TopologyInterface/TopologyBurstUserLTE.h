/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/1 3:41 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : TopologyRandUserLTE.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_TOPOLOGYRANDUSERLTE_H
#define WORKER_TOPOLOGYRANDUSERLTE_H

#include "TopologyInterface.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/lte-module.h"
#include "ErrorModel/user-uplink/user-uplink-congestion.h"

namespace ns3 {

	class TopologyBurstUserLTE : public TopologyInterface,
		public PointToPointHelper {
	public:
		TopologyBurstUserLTE();
		~TopologyBurstUserLTE() {}

		NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);

	private:
		Ptr<LteHelper> lte_helper_; // lte 协议
		Ptr<PointToPointEpcHelper> epc_helper_; // epc 核心网
		Ptr<Node> pgw_; // pdn 网关
	};

}  // namespace ns3


#endif // WORKER_TOPOLOGYRANDUSERLTE_H

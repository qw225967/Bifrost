/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/1/11 2:01 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : TopologySimpleLTE.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_TOPOLOGYSIMPLELTE_H
#define WORKER_TOPOLOGYSIMPLELTE_H

#include "TopologyInterface.h"

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/lte-module.h"

namespace ns3 {
class TopologySimpleLTE : public TopologyInterface {
 public:
	TopologySimpleLTE();
	~TopologySimpleLTE() {}

  NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);

 private:
  Ptr<LteHelper> lte_helper_; // lte 协议
  Ptr<PointToPointEpcHelper> epc_helper_; // epc 核心网
  Ptr<Node> pgw_; // pdn 网关
};
}  // namespace ns3

#endif  // WORKER_TOPOLOGYSIMPLELTE_H

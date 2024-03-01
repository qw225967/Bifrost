
#ifndef _TOPOLOGYWIFIMODULE_H
#define _TOPOLOGYWIFIMODULE_H

#include "TopologyInterface.h"
#include "ns3/core-module.h"

namespace ns3 {

class TopologyWifiModule : public TopologyInterface {
 public:
  TopologyWifiModule() {}
  ~TopologyWifiModule() {}

  NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);
};

}  // namespace ns3

#endif  //_TOPOLOGYWIFIMODULE_H

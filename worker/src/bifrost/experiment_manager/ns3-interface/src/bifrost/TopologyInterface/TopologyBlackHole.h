/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/1/5 2:40 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : TopologyBlackhole.h
 * @description : TODO
 *******************************************************/

#ifndef _TOPOLOGYBLACKHOLE_H
#define _TOPOLOGYBLACKHOLE_H

#include "TopologyInterface.h"

#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ErrorModel/blackhole/blackhole-error-model.h"

namespace ns3 {

class TopologyBlackHole : public TopologyInterface,
                          public PointToPointHelper {
 public:
  TopologyBlackHole() {}
  ~TopologyBlackHole() {}

  NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);
};

}  // namespace ns3

#endif  //_TOPOLOGYBLACKHOLE_H

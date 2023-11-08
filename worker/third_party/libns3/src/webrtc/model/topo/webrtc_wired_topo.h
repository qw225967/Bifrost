/******************************************************************************
 * Copyright 2016-2017 Cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 *                                                                            *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/

/**
 * @file
 * Wired network topology setup for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#ifndef WIRED_TOPO_H
#define WIRED_TOPO_H

#include "webrtc_topo.h"
#include <stdint.h>
#include "ns3/application-container.h"
#include <string>
namespace ns3 {

/**
 * Class implementing the network Topology for rmcat wired test cases.
 * The diagram below depicts the topology and the IP subnets configured.
 *
 * +----+ 10.0.1.0                        11.0.1.0 +----+
 * | l1 +-----------+                  +-----------+ r1 |
 * +----+           |                  |           +----+
 *                  |                  |
 * +----+ 10.0.2.0 +---+  12.0.1.0  +---+ 11.0.2.0 +----+
 * | l2 +----------+ A +------------+ B +----------+ r2 |
 * +----+          +---+            +---+          +----+
 *                  |                  |
 *  ...             |                  |            ...
 *                  |                  |
 * +----+ 10.x.y.0  |                  |  11.x.y.0 +----+
 * | ln +-----------+                  +-----------+ rn |
 * +----+                                          +----+
 * where n = 256 * x + y
 */

class WebrtcWiredTopo: public WebrtcTopo
{
public:
    /** Class constructor */
    WebrtcWiredTopo ();

    /** Class destructor */
    virtual ~WebrtcWiredTopo ();

    /**
     * Build the wired rmcat topology with the attributes passed
     *
     * @param [in] bandwidthBps Bandwidth of the bottleneck link (in bps)
     * @param [in] msDelay Total propagation delay (in ms) between left and
     *                     right nodes
     * @param [in] msQDelay Capacity of the queue at the bottleneck
     *                      link (in ms)
     */
    void Build (uint64_t bandwidthBps, uint32_t msDelay, uint32_t msQDelay);

    /**
     * Install a one-way bulk TCP flow in a pair of (left-to-right) nodes
     *
     * @param [in] flowId A string denoting the flow's id. Useful for logging
     *                    and plotting
     * @param [in] serverPort TCP port where the server bulk TCP application is
     *                        to listen
     * @param [in] newNode If true, the (sender/receiver) bulk TCP applications
     *                     will be installed in a newly created (left-right)
     *                     node pair; if false, the previously created node
     *                     pair will be reused
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallTCP (const std::string& flowId,
                                     uint16_t serverPort,
                                     bool newNode);

    /**
     * Install a one-way constant bitrate (CBR) UDP flow in a pair of
     * (left-right) nodes
     *
     * @param [in] serverPort UDP port where the receiver CBR UDP application
     *                        is to receive datagrams
     * @param [in] bitrate Bitrate (constant) at which the flow is to operate
     * @param [in] packetSize Size of of the data to be shipped in each datagram
     * @param [in] forward  direction of the flow.
     *             If true (forward direction), the left node (ID=0)
     *             will act as sender and the right node (ID=1) will
     *             act as receiver; if false (backward direction),
     *             the roles are swapped.
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallCBR (uint16_t serverPort,
                                     uint32_t bitrate = 0,
                                     uint32_t packetSize = DEFAULT_PACKET_SIZE,
                                     bool forward = true);

    /**
     * Install a one-way rmcat flow in a pair of (left-right) nodes
     *
     * @param [in] idx Index of this rmcat flow, in order to distinguish it
     *                 from other flows in the logs and plots
     * @param [in] serverPort UDP port where the #RmcatReceiver application
     *                        is to receive media packets
     * @param [in] pDelayMs Custom propagation delay between the pair of nodes
     * @param [in] forward  direction of flow.
     *             If true (forward direction), the left node (ID=0)
     *             will act as sender and the right node (ID=1) will
     *             act as receiver; if false (backward direction),
     *             the roles are swapped.
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallWebrtc (const std::string& flowId,
                                       uint16_t serverPort,
                                       uint32_t pDelayMs,
                                       bool forward,
                                       double initBw,
                                       double minBw,
                                       double maxBw);

private:
    void SetupAppNode (Ptr<Node> node, int subnet, uint32_t pDelayMs);
    NodeContainer SetupAppNodes (uint32_t pDelayMs, bool newNode);

protected:
    unsigned m_numApps;
    uint32_t m_bufSize;
    NodeContainer m_bottleneckNodes;
    NodeContainer m_appNodes; // Last application node pair created
    NetDeviceContainer m_bottleneckDevices;
    InternetStackHelper m_inetStackHlpr;
    PointToPointHelper m_appLinkHlpr;
};

}

#endif /* WIRED_TOPO_H */

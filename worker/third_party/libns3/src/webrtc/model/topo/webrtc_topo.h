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
 * Common network topology setup for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#ifndef TOPO_H
#define TOPO_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-trace-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/wifi-module.h"
#include "ns3/traffic-control-helper.h"

#include "ns3/webrtc-constants.h"

namespace ns3 {

class WebrtcTopo
{
protected:
    /**
     * Install two applications (sender and receiver) implementing a TCP flow.
     * The sender of application data (resp. receiver) will be installed at the
     * sender (resp. receiver) node.
     *
     * @param [in]     flowId A string denoting the flow's id. Useful for
     *                        logging and plotting
     * @param [in,out] sender ns3 node that is to contain the sender bulk TCP
     *                        application
     * @param [in,out] receiver ns3 node that is to contain the receiver bulk
     *                          TCP application
     * @param [in]     serverPort TCP port where the server application is
     *                            to listen
     *
     * @retval A container with the two applications (sender and receiver)
     *
     */
    static ApplicationContainer InstallTCP (const std::string& flowId,
                                            Ptr<Node> sender,
                                            Ptr<Node> receiver,
                                            uint16_t serverPort);

    /**
     * Install two applications (sender and receiver) implementing a constant
     * bitrate (CBR) flow over UDP.
     * The sender of application data (resp. receiver) will be installed at the
     * sender (resp. receiver) node.
     *
     * @param [in,out] sender ns3 node that is to contain the sender CBR UDP
     *                        application
     * @param [in,out] receiver ns3 node that is to contain the receiver CBR
     *                          UDP application
     * @param [in]     serverPort UDP port where the receiver application is
     *                            to read datagrams
     * @param [in]     bitrate Bitrate (constant) at which the flow is to
     *                         operate
     * @param [in]     packetSize Size of of the data to be shipped in each
     *                            datagram
     *
     * @retval A container with the two applications (sender and receiver)
     *
     */
    static ApplicationContainer InstallCBR (Ptr<Node> sender,
                                            Ptr<Node> receiver,
                                            uint16_t serverPort,
                                            uint64_t bitrate,
                                            uint32_t packetSize = DEFAULT_PACKET_SIZE);


    /**
     * Install two applications (sender and receiver) implementing an RMCAT
     * flow.
     * The sender of application data (resp. receiver) will be installed at the
     * sender (resp. receiver) node.
     *
     * @param [in]     flowId A string denoting the flow's id. Useful for
     *                        logging and plotting
     * @param [in,out] sender ns3 node that is to contain the #RmcatSender
     *                        application
     * @param [in,out] receiver ns3 node that is to contain the #RmcatReceiver
     *                          application
     * @param [in]     serverPort UDP port where the receiver application is
     *                            to read media packets
     *
     * @retval A container with the two applications (sender and receiver)
     */

    static ApplicationContainer InstallWebrtc (const std::string& flowId,
                                              Ptr<Node> sender,
                                              Ptr<Node> receiver,
                                              uint16_t serverPort,
                                              double initBw,
                                              double minBw,
                                              double maxBw);


    /**
     * Simple logging callback to be passed to the congestion controller
     *
     * @param [in] msg Message that the congestion controller wants to log
     */
    static void logFromController (const std::string& msg);
};

}

#endif /* TOPO_H */

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
 * Wifi network topology setup for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#ifndef WIFI_TOPO_H
#define WIFI_TOPO_H

#include "webrtc_topo.h"
namespace ns3 {

/**
 * Class implementing the network Topology for rmcat Wifi test cases.
 * The diagram below depicts the topology and the IP subnets configured.
 *
 * Infrastructure Wifi Network Topology
 *
 *     <===== upstream direction =========
 *
 *                        Wifi 10.1.2.0
 *                                           APs
 *                      *    *    *  ...   *
 *        10.1.1.0      |    |    |  ...   |
 *  n1 --------------- n2   n3   n4  ...   n0
 *      point-to-point
 *
 *    ======= downstream direction =====>
 *
 */

class WebrtcWifiTopo: public WebrtcTopo
{
public:
    /** Class destructor */
    virtual ~WebrtcWifiTopo ();

    /**
     * Build the wifi rmcat topology with the attributes passed
     *
     * The default euclidean position of the AP is (0, 0, 0).
     * The default euclidean position of all wifi station nodes is (1, 0, 0),
     * in meters
     *
     * @param [in] bandwidthBps Bandwidth of the wired link (in bps)
     * @param [in] msDelay Propagation delay of the wired link (in ms)
     * @param [in] msQDelay Capacity of the ingress queue at the wired link
     *                      (in ms), also maximum delay attribute of the wifi
     *                      max queue
     * @param [in] nWifi Number of wifi nodes, not counting the AP (i.e.,
     *                   wifi stations)
     * @param [in] standard Wifi standard to use. See #WifiPhyStandard .
     * @param [in] rateMode Wifi transmission mode.
     *              dynamic rateMode will use Minstrel rate adaptation algorithm
     *              constant rateMode for 802.11 abg, Configure80211* function, such as "DsssRate11Mbps"
     *                       @see https://www.nsnam.org/doxygen/yans-wifi-phy_8cc_source.html
     *              constant rateMode for 802.11 n/ac, such as "HtMcs23"
     *                       @see https://www.nsnam.org/doxygen/wifi-phy_8cc_source.html#l01627,
     *              @see http://mcsindex.com/ for data rate corresponding to specific mcsidx
     *              @see http://www.digitalairwireless.com/wireless-blog/recent/demystifying-modulation-and-coding-scheme-index-values.html
     *                   for details
     */
    void Build (uint64_t bandwidthBps,
                uint32_t msDelay,
                uint32_t msQDelay,
                uint32_t nWifi,
                WifiStandard standard,
                WifiPhyBand band,
                WifiMode rateMode);

    /**
     * Install a one-way TCP flow in a pair of nodes: a wifi node and a
     * wired node
     *
     * @param [in] flowId     A string denoting the flow's id. Useful for
     *                        logging and plotting
     * @param [in] nodeId     index of the wifi node where the TCP flow is
     *                        to be installed.
     * @param [in] serverPort TCP port where the server TCP application is
     *                        to listen on
     * @param [in] downstream If true, the wifi node is to act as receiver
     *                        (downstream direction); if false, the wifi node
     *                        is to act as sender (upstream direction)
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallTCP (const std::string& flowId,
                                     uint32_t nodeId,
                                     uint16_t serverPort,
                                     bool downstream);

    /**
     * Install a one-way constant bitrate (CBR) flow over UDP in a pair of nodes:
     * a wifi node and a wired node
     *
     * @param [in] nodeId      index of the wifi node where the CBR-over-UDP
     *                         flow is to be installed.
     * @param [in] serverPort UDP port where the receiver CBR UDP application
     *                        is to receive datagrams
     * @param [in] bitrate Bitrate (constant) at which the flow is to operate
     * @param [in] packetSize Size of of the data to be shipped in each
     *                        datagram
     * @param [in] downstream If true, the wifi node is to act as receiver
     *                        (downstream direction); if false, the wifi node
     *                        is to act as sender (upstream direction)
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallCBR (uint32_t nodeId,
                                     uint16_t serverPort,
                                     uint64_t bitrate,
                                     uint32_t packetSize,
                                     bool downstream);

    /**
     * Install a one-way RMCAT flow in a pair of nodes: a wifi node and a
     * wired node
     *
     * @param [in] flowId     A string denoting the flow's id. Useful for
     *                        logging and plotting
     * @param [in] nodeId     index of the wifi node where the RMCAT flow
     *                        is to be installed.
     * @param [in] serverPort UDP port where the #RmcatReceiver application
     *                        is to receive media packets
     * @param [in] downstream If true, the wifi node is to act as receiver
     *                        (downstream direction); if false, the wifi node
     *                        is to act as sender (upstream direction)
     *
     * @retval A container with the two applications (sender and receiver)
     */
    ApplicationContainer InstallWebrtc (const std::string& flowId,
                                       uint32_t nodeId,
                                       uint16_t serverPort,
                                       bool downstream,
                                       double initBw,
                                       double minBw,
                                       double maxBw);

    /**
     * Get the current euclidean position of a wifi station node
     *
     * @param [in] idx Index of the wifi node
     * @retval A vector object containing the wifi station's (x, y, z)
     *         coordinates
     */
    Vector GetPosition (uint32_t idx) const;

    /**
     * Set the euclidean position of a wifi station node
     *
     * @param [in] idx Index of the wifi node
     * @param [in] position New position of the wifi node
     */
    void SetPosition (uint32_t idx, const Vector& position);

private:
    static void EnableWifiLogComponents ();

protected:
    WifiHelper m_wifi;
    NodeContainer m_wiredNodes;
    NodeContainer m_wifiStaNodes;
    NodeContainer m_wifiApNode;
    NetDeviceContainer m_wiredDevices;
    NetDeviceContainer m_staDevices;
    NetDeviceContainer m_apDevices;
};

}

#endif /* WIFI_TOPO_H */

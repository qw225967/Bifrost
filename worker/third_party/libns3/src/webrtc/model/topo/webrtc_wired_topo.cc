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
 * Wired network topology setup implementation for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "webrtc_wired_topo.h"

namespace ns3 {

WebrtcWiredTopo::WebrtcWiredTopo ()
: m_numApps{0},
  m_bufSize{0}
{}

WebrtcWiredTopo::~WebrtcWiredTopo ()
{}

void WebrtcWiredTopo::Build (uint64_t bandwidthBps, uint32_t msDelay, uint32_t msQDelay)
{
    // Set up bottleneck link
    m_bottleneckNodes.Create (2);
    PointToPointHelper bottleneckLinkHlpr;
    bottleneckLinkHlpr.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (bandwidthBps)));

    // We set the the bottleneck link's propagation delay to 90% of the total delay
    bottleneckLinkHlpr.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (msDelay * 1000 * 9 / 10)));
    m_bufSize = bandwidthBps * msQDelay / 8 / 1000;
    // At least one full packet with default size must fit
    NS_ASSERT (m_bufSize >= DEFAULT_PACKET_SIZE + IPV4_UDP_OVERHEAD);

    uint32_t packetSize = std::max(1, static_cast<int>(m_bufSize / DEFAULT_PACKET_SIZE));
    bottleneckLinkHlpr.SetQueue ("ns3::DropTailQueue",                                 
                                 "MaxSize", StringValue(std::to_string(packetSize) + "p"));

    m_bottleneckDevices = bottleneckLinkHlpr.Install (m_bottleneckNodes);

    //Uncomment the line below to ease troubleshooting
    //bottleneckLinkHlpr.EnablePcapAll ("rmcat-wired-capture", true);

    m_inetStackHlpr.Install (m_bottleneckNodes);
    Ipv4AddressHelper address;
    address.SetBase ("12.0.1.0", "255.255.255.0");
    address.Assign (m_bottleneckDevices);

    // Set up helpers for applications
    NS_ASSERT (m_numApps == 0);
    m_appLinkHlpr.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (1u << 30))); // 1 Gbps

    // We set the application link's propagation delay (one on each side of the bottleneck)
    // to 5% of the total delay
    m_appLinkHlpr.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (msDelay * 1000 * 5 / 100)));

    // Set queue to drop-tail, but don't care much about buffer size
    m_appLinkHlpr.SetQueue ("ns3::DropTailQueue",                            
                            "MaxSize", StringValue(std::to_string(packetSize) + "p"));

    // Disable tc now, some bug in ns3 causes extra delay
    TrafficControlHelper tch;
    tch.Uninstall (m_bottleneckDevices);

    Packet::EnablePrinting ();
}

ApplicationContainer WebrtcWiredTopo::InstallTCP (const std::string& flowId,
                                            uint16_t serverPort,
                                            bool newNode)
{
    auto appNodes = SetupAppNodes (0, newNode);
    auto sender = appNodes.Get (0);
    auto receiver = appNodes.Get (1);

    return WebrtcTopo::InstallTCP (flowId, sender, receiver, serverPort);
}

ApplicationContainer WebrtcWiredTopo::InstallCBR (uint16_t serverPort,
                                            uint32_t bitrate,
                                            uint32_t packetSize,
                                            bool forward)
{
    // At least one packet must fit in the bottleneck link's queue
    NS_ASSERT (m_bufSize >= packetSize + IPV4_UDP_OVERHEAD);

    auto appNodes = SetupAppNodes (0, true);
    auto sender = appNodes.Get (1);
    auto receiver = appNodes.Get (0);
    if (forward) {
        std::swap (sender, receiver);
    }

    return WebrtcTopo::InstallCBR (sender,
                             receiver,
                             serverPort,
                             bitrate,
                             packetSize);
}

ApplicationContainer WebrtcWiredTopo::InstallWebrtc (const std::string& flowId,
                                              uint16_t serverPort,
                                              uint32_t pDelayMs,
                                              bool forward,
                                              double initBw,
                                              double minBw,
                                              double maxBw)
{
    auto appNodes = SetupAppNodes (pDelayMs, true);

    auto sender = appNodes.Get (1);
    auto receiver = appNodes.Get (0);
    if (forward) {
        std::swap (sender, receiver);
    }

    return WebrtcTopo::InstallWebrtc (flowId,
                               sender,
                               receiver,
                               serverPort,
                               initBw,
                               minBw,
                               maxBw);
}

void WebrtcWiredTopo::SetupAppNode (Ptr<Node> node, int bottleneckIdx, uint32_t pDelayMs)
{
    NodeContainer nodes (node, m_bottleneckNodes.Get (bottleneckIdx));
    auto devices = m_appLinkHlpr.Install (nodes);
    if (pDelayMs > 0) {
        Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel> (devices.Get (0)->GetChannel ());
        TimeValue delay;
        channel->GetAttribute ("Delay", delay);
        const uint64_t us = delay.Get ().GetMicroSeconds ();
        // variable us is 5% of the total base delay
        const uint64_t total = us * 100 / 5;
        NS_ASSERT (us <= total);
        NS_ASSERT (total - us <= pDelayMs * 1000);
        delay.Set (MicroSeconds (pDelayMs * 1000 - (total - us)));
        channel->SetAttribute ("Delay", delay);
    }
    Ipv4AddressHelper address;
    std::ostringstream stringStream;
    const unsigned x = (m_numApps + 1) / 256;
    const unsigned y = (m_numApps + 1) % 256;
    stringStream << (10 + bottleneckIdx) << "." << x << "." << y << ".0";
    address.SetBase (stringStream.str ().c_str (), "255.255.255.0");
    address.Assign (devices);

    TrafficControlHelper tch;
    tch.Uninstall (devices);

    //Uncomment the lines below to ease troubleshooting
    //if (bottleneckIdx == 0) {
    //    std::ostringstream stringStream0;
    //    stringStream0 << "rmcat-wired-app-capture" << m_numApps;
    //    m_appLinkHlpr.EnablePcap (stringStream0.str (), devices, true);
    //}
}

NodeContainer WebrtcWiredTopo::SetupAppNodes (uint32_t pDelayMs, bool newNode)
{
    if (newNode) {
        NodeContainer appNodes;
        appNodes.Create (2);
        m_inetStackHlpr.Install (appNodes);
        SetupAppNode (appNodes.Get (0), 0, pDelayMs);
        SetupAppNode (appNodes.Get (1), 1, 0);
        ++m_numApps;
        m_appNodes = appNodes;
    }
    // The first time we set up a flow, newNode must be true
    NS_ASSERT (m_numApps > 0);
    return m_appNodes;
}

}

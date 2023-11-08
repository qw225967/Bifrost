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
 * Common network topology setup implementation for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "webrtc_topo.h"
#include "ns3/webrtc-sender.h"
#include "ns3/webrtc-receiver.h"
#include "ns3/common.h"
#include <memory>
#include <limits>
#include <sys/stat.h>
#include "ns3/goog_cc_network_controller.h"

NS_LOG_COMPONENT_DEFINE ("Topo");

namespace ns3 {

/* Implementations of utility functions */

static Ipv4Address GetIpv4AddressOfNode (Ptr<Node> node,
                                         uint32_t interface,
                                         uint32_t addressIndex)
{
    // interface index 0 is the loopback
    auto ipv4 = node->GetObject<Ipv4> ();
    auto iaddr = ipv4->GetAddress (interface, addressIndex);
    return iaddr.GetLocal ();
}


static std::string GetPrefix (Ptr<Node> node,
                              const std::string& logName,
                              const std::string& flowId,
                              uint16_t port=0)
{
    std::ostringstream os;
    os << "[";
    GetIpv4AddressOfNode (node, 1, 0).Print (os);
    os << ":" << port << "] " << logName << ": " << flowId;
    return os.str ();
}

static Time GetIntervalFromBitrate (uint64_t bitrate, uint32_t packetSize)
{
    if (bitrate == 0u) {
        return Time::Max ();
    }
    const auto secs = static_cast<double> (packetSize + IPV4_UDP_OVERHEAD) /
                            (static_cast<double> (bitrate) / 8. );
    return Seconds (secs);
}

static void PacketSinkLogging (const std::string& nameAndId,
                               Ptr<Application> app,
                               double interval,
                               uint32_t oldRecv)
{
    TimeValue startT, stopT;
    app->GetAttribute ("StartTime", startT);
    app->GetAttribute ("StopTime", stopT);
    const auto now = Simulator::Now ().GetMilliSeconds ();
    const auto start = startT.Get ().GetMilliSeconds ();
    const auto stop = stopT.Get ().GetMilliSeconds ();
    auto recv = oldRecv;
    if (start <= now &&  now <= stop) {
        Ptr<PacketSink> sink = DynamicCast<PacketSink> (app);
        recv = sink->GetTotalRx ();
        std::ostringstream os;
        os << std::fixed;
        os.precision (4);
        os << nameAndId << " ts: " << now
           << " recv: " << recv // Number of bytes received so far for this flow
           << " rrate: " << static_cast<double> (recv - oldRecv) / interval * 8.;
                                                // Subtraction will wrap properly
        NS_LOG_INFO (os.str ());
    }
    Time tNext{Seconds (interval)};
    Simulator::Schedule (tNext, &PacketSinkLogging, nameAndId, app, interval, recv);
}


/*
 * Implementations of:
 * -- InstallTCP
 * -- InstallCBR
 * -- InstallRMCAT
 */

ApplicationContainer WebrtcTopo::InstallTCP (const std::string& flowId,
                                       Ptr<Node> sender,
                                       Ptr<Node> receiver,
                                       uint16_t serverPort)
{
    BulkSendHelper source ("ns3::TcpSocketFactory",
                           InetSocketAddress{GetIpv4AddressOfNode (receiver, 1, 0), serverPort});

    // Set the amount of data to send in bytes. Zero denotes unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetAttribute ("SendSize", UintegerValue (DEFAULT_PACKET_SIZE));

    auto clientApps = source.Install (sender);
    clientApps.Start (Seconds (0));
    clientApps.Stop (Seconds (T_MAX_S));

    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           InetSocketAddress{Ipv4Address::GetAny (), serverPort});
    auto serverApps = sink.Install (receiver);
    serverApps.Start (Seconds (0));
    serverApps.Stop (Seconds (T_MAX_S));

    const auto interval = T_TCP_LOG;
    Simulator::Schedule (Seconds (interval),
                         &PacketSinkLogging,
                         GetPrefix (sender, "tcp_log", flowId, serverPort),
                         serverApps.Get (0),
                         interval,
                         0);

    ApplicationContainer apps;
    apps.Add (clientApps);
    apps.Add (serverApps);
    return apps;
}


ApplicationContainer WebrtcTopo::InstallCBR (Ptr<Node> sender,
                                       Ptr<Node> receiver,
                                       uint16_t serverPort,
                                       uint64_t bitrate,
                                       uint32_t packetSize)
{
    UdpServerHelper server (serverPort);
    ApplicationContainer serverApps = server.Install (receiver);
    serverApps.Start (Seconds (0));
    serverApps.Stop (Seconds (T_MAX_S));

    const auto interPacketInterval = GetIntervalFromBitrate (bitrate, packetSize);
    const auto maxPacketCount = std::numeric_limits<uint32_t>::max ();
    UdpClientHelper client (GetIpv4AddressOfNode (receiver, 1, 0), serverPort);
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));
    ApplicationContainer clientApps = client.Install (sender);
    clientApps.Start (Seconds (0));
    clientApps.Stop (Seconds (T_MAX_S));

    ApplicationContainer apps;
    apps.Add (clientApps);
    apps.Add (serverApps);
    return apps;
}

ApplicationContainer WebrtcTopo::InstallWebrtc (const std::string& flowId,
                                         Ptr<Node> sender,
                                         Ptr<Node> receiver,
                                         uint16_t serverPort,
                                         double initBw,
                                         double minBw,
                                         double maxBw
                                         )
{

    auto webrtcAppSend = CreateObject<WebrtcSender> ();
    auto webrtcAppRecv = CreateObject<WebrtcReceiver> ();
    sender->AddApplication(webrtcAppSend);
    receiver->AddApplication (webrtcAppRecv);

    Ipv4Address serverIP = GetIpv4AddressOfNode (receiver, 1, 0);
    webrtcAppSend->Setup (serverIP, serverPort, initBw, minBw, maxBw);

    /* configure congestion controller */
    //auto controller = std::make_unique<GoogCcNetworkController> ();    
    //controller->setId (flowId); 
    webrtcAppSend->SetController (nullptr);

    webrtcAppSend->SetStartTime (Seconds (0));
    webrtcAppSend->SetStopTime (Seconds (T_MAX_S));

    webrtcAppRecv->Setup (serverPort);
    webrtcAppRecv->SetStartTime (Seconds (0));
    webrtcAppRecv->SetStopTime (Seconds (T_MAX_S));

    ApplicationContainer apps;
    apps.Add (webrtcAppSend);
    apps.Add (webrtcAppRecv);
    return apps;
}

void WebrtcTopo::logFromController (const std::string& msg) {
    NS_LOG_INFO ("controller_log: " << msg);
}

}

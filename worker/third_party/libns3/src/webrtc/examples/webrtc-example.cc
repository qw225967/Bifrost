/******************************************************************************
 * Copyright 2016-2017 cisco Systems, Inc.                                    *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
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
 * Simple example demonstrating the usage of the rmcat ns3 module, using:
 *  - NADA as controller for rmcat flows
 *  - Statistics-based traffic source as codec
 *  - [Optionally] TCP flows
 *  - [Optionally] UDP flows
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "ns3/goog_cc_network_controller.h"
#include "ns3/webrtc-sender.h"
#include "ns3/webrtc-receiver.h"
#include "ns3/webrtc-constants.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/data-rate.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/core-module.h"
#include <memory>

const uint32_t WEBRTC_DEFAULT_RMIN  =  150000;  // in bps: 150Kbps
const uint32_t WEBRTC_DEFAULT_RMAX  = 1500000;  // in bps: 1.5Mbps
const uint32_t WEBRTC_DEFAULT_RINIT =  150000;  // in bps: 150Kbps

const uint32_t TOPO_DEFAULT_BW     = 1000000;    // in bps: 1Mbps
const uint32_t TOPO_DEFAULT_PDELAY =      50;    // in ms:   50ms
const uint32_t TOPO_DEFAULT_QDELAY =     300;    // in ms:  300ms

using namespace ns3;
static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    auto packetSize = std::max(1, static_cast<int>(bufSize / DEFAULT_PACKET_SIZE));
    pointToPoint.SetQueue ("ns3::DropTailQueue",                           
                           "MaxSize", StringValue(std::to_string(packetSize) + "p"));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("10.1.1.0", "255.255.255.0");
    address.Assign (devices);

    // Uncomment to capture simulated traffic
    // pointToPoint.EnablePcapAll ("rmcat-example");

    // disable tc for now, some bug in ns3 causes extra delay
    TrafficControlHelper tch;
    tch.Uninstall (devices);

    return nodes;
}

static void InstallTCP (Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t port,
                        float startTime,
                        float stopTime)
{
    // configure TCP source/sender/client
    auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    BulkSendHelper source{"ns3::TcpSocketFactory",
                           InetSocketAddress{serverAddr, port}};
    // Set the amount of data to send in bytes. Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (0));
    source.SetAttribute ("SendSize", UintegerValue (DEFAULT_PACKET_SIZE));

    auto clientApps = source.Install (sender);
    clientApps.Start (Seconds (startTime));
    clientApps.Stop (Seconds (stopTime));

    // configure TCP sink/receiver/server
    PacketSinkHelper sink{"ns3::TcpSocketFactory",
                           InetSocketAddress{Ipv4Address::GetAny (), port}};
    auto serverApps = sink.Install (receiver);
    serverApps.Start (Seconds (startTime));
    serverApps.Stop (Seconds (stopTime));

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

static void InstallUDP (Ptr<Node> sender,
                        Ptr<Node> receiver,
                        uint16_t serverPort,
                        uint64_t bitrate,
                        uint32_t packetSize,
                        uint32_t startTime,
                        uint32_t stopTime)
{
    // configure UDP source/sender/client
    auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal ();
    const auto interPacketInterval = GetIntervalFromBitrate (bitrate, packetSize);
    uint32_t maxPacketCount = 0XFFFFFFFF;
    UdpClientHelper client{serverAddr, serverPort};
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));

    auto clientApps = client.Install (sender);
    clientApps.Start (Seconds (startTime));
    clientApps.Stop (Seconds (stopTime));

    // configure TCP sink/receiver/server
    UdpServerHelper server{serverPort};
    auto serverApps = server.Install (receiver);
    serverApps.Start (Seconds (startTime));
    serverApps.Stop (Seconds (stopTime));
}

static void InstallApps (bool webrtc,
                         Ptr<Node> sender,
                         Ptr<Node> receiver,
                         uint16_t port,
                         float initBw,
                         float minBw,
                         float maxBw,
                         float startTime,
                         float stopTime)
{
    Ptr<WebrtcSender> sendApp = CreateObject<WebrtcSender> ();
    Ptr<WebrtcReceiver> recvApp = CreateObject<WebrtcReceiver> ();
    sender->AddApplication (sendApp);
    receiver->AddApplication (recvApp);

    if (webrtc) {
        TargetRateConstraints constaints;
        constaints.at_time = Timestamp::Millis(Simulator::Now().GetMilliSeconds());
        constaints.starting_rate = RtcDataRate::BitsPerSec(initBw);
        constaints.min_data_rate = RtcDataRate::BitsPerSec(minBw);
        constaints.max_data_rate = RtcDataRate::BitsPerSec(maxBw);
        sendApp->SetController (std::make_unique<GoogCcNetworkController> (constaints));
    }
    Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
    Ipv4Address receiverIp = ipv4->GetAddress (1, 0).GetLocal ();  
    
    sendApp->SetCodecType (SyncodecType::SYNCODEC_TYPE_TRACE);
    sendApp->Setup (receiverIp, port,initBw, minBw, maxBw);

    recvApp->Setup (port);

    sendApp->SetStartTime (Seconds (startTime));
    sendApp->SetStopTime (Seconds (stopTime));

    recvApp->SetStartTime (Seconds (startTime));
    recvApp->SetStopTime (Seconds (stopTime));
}

int main (int argc, char *argv[])
{
    int nRmcat = 1;
    int nTcp = 0;
    int nUdp = 0;
    bool log = false;
    bool nada = true;
    std::string strArg  = "strArg default";

    CommandLine cmd;
    cmd.AddValue ("rmcat", "Number of RMCAT (NADA) flows", nRmcat);
    cmd.AddValue ("tcp", "Number of TCP flows", nTcp);
    cmd.AddValue ("udp", "Number of UDP flows", nUdp);
    cmd.AddValue ("log", "Turn on logs", log);
    cmd.AddValue ("nada", "true: use NADA, false: use dummy", nada);
    cmd.Parse (argc, argv);

    if (log) {
        LogComponentEnable ("RmcatSender", LOG_INFO);
        LogComponentEnable ("RmcatReceiver", LOG_INFO);
        LogComponentEnable ("Packet", LOG_FUNCTION);
    }

    // configure default TCP parameters
    Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1000));

    const uint64_t linkBw   = TOPO_DEFAULT_BW;
    const uint32_t msDelay  = TOPO_DEFAULT_PDELAY;
    const uint32_t msQDelay = TOPO_DEFAULT_QDELAY;

    const float minBw =  WEBRTC_DEFAULT_RMIN;
    const float maxBw =  WEBRTC_DEFAULT_RMAX;
    const float initBw = WEBRTC_DEFAULT_RINIT;

    const float endTime = 300.;

    NodeContainer nodes = BuildExampleTopo (linkBw, msDelay, msQDelay);

    int port = 8000;
    nRmcat = std::max<int> (0, nRmcat); // No negative RMCAT flows
    for (size_t i = 0; i < (unsigned int) nRmcat; ++i) {
        auto start = 10. * i;
        auto end = std::max (start + 1., endTime - start);
        InstallApps (nada, nodes.Get (0), nodes.Get (1), port++,
                     initBw, minBw, maxBw, start, end);
    }

    nTcp = std::max<int> (0, nTcp); // No negative TCP flows
    for (size_t i = 0; i < (unsigned int) nTcp; ++i) {
        auto start = 17. * i;
        auto end = std::max (start + 1., endTime - start);
        InstallTCP (nodes.Get (0), nodes.Get (1), port++, start, end);
    }

    // UDP parameters
    const uint64_t bandwidth = WEBRTC_DEFAULT_RMAX / 4;
    const uint32_t pktSize = DEFAULT_PACKET_SIZE;

    nUdp = std::max<int> (0, nUdp); // No negative UDP flows
    for (size_t i = 0; i < (unsigned int) nUdp; ++i) {
        auto start = 23. * i;
        auto end = std::max (start + 1., endTime - start);
        InstallUDP (nodes.Get (0), nodes.Get (1), port++,
                    bandwidth, pktSize, start, end);
    }

    std::cout << "Running Simulation..." << std::endl;
    Simulator::Stop (Seconds (endTime));
    Simulator::Run ();
    Simulator::Destroy ();
    std::cout << "Done" << std::endl;

    return 0;
}

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

#include "ns3/webrtc_wifi_topo.h"
#include <utility>

namespace ns3 {

WebrtcWifiTopo::~WebrtcWifiTopo () {}

void WebrtcWifiTopo::Build (uint64_t bandwidthBps,
                      uint32_t msDelay,
                      uint32_t msQDelay,
                      uint32_t nWifi,
                      WifiStandard standard,
                      WifiPhyBand band,
                      WifiMode rateMode)
{
    // Set up wired link
    m_wiredNodes.Create (2);
    PointToPointHelper wiredLinkHlpr;
    wiredLinkHlpr.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (bandwidthBps)));
    wiredLinkHlpr.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));

    uint32_t bufSize = bandwidthBps * msQDelay / 8 / 1000;
    // At least one full packet with default size must fit
    NS_ASSERT (bufSize >= DEFAULT_PACKET_SIZE + IPV4_UDP_OVERHEAD);
    //StringValue(std::to_string(netdevicesQueueSize) + "p")
    uint32_t packetSize = std::max(1, static_cast<int>(bufSize / DEFAULT_PACKET_SIZE));
    wiredLinkHlpr.SetQueue ("ns3::DropTailQueue",                            
                            "MaxSize", StringValue(std::to_string(packetSize) + "p"));

    m_wiredDevices = wiredLinkHlpr.Install (m_wiredNodes);

    //Uncomment the line below to ease troubleshooting
    //wiredLinkHlpr.EnablePcapAll ("WifiTopo-wired");

    // Config global settings for wifi mac queue
    Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (WIFI_TOPO_MACQUEUE_MAXNPKTS));
    Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (msQDelay)));

    // Workaround for the arp bug, https://groups.google.com/forum/#!topic/ns-3-users/TiY9IUFnrZ4
    // http://mailman.isi.edu/pipermail/ns-developers/2007-November/003549.html
    // https://www.nsnam.org/bugzilla/show_bug.cgi?id=2057
    Config::SetDefault ("ns3::ArpCache::AliveTimeout", TimeValue (Seconds (WIFI_TOPO_ARPCACHE_ALIVE_TIMEOUT)));

    m_wifiStaNodes.Create (nWifi);
    m_wifiApNode = m_wiredNodes.Get (0);

    // Create channel
    YansWifiPhyHelper wifiPhyHelper;// = YansWifiPhyHelper::Default ();
    wifiPhyHelper.Set ("ChannelNumber",UintegerValue (6));
    YansWifiChannelHelper wifiChannelHelper = YansWifiChannelHelper::Default ();
    wifiChannelHelper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

    // reference loss must be changed when operating at 2.4GHz
    if (standard == WIFI_STANDARD_80211n && band == WIFI_PHY_BAND_2_4GHZ) {
        wifiChannelHelper.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                              "Exponent", DoubleValue (WIFI_TOPO_2_4GHZ_PATHLOSS_EXPONENT),
                                              "ReferenceLoss", DoubleValue (WIFI_TOPO_2_4GHZ_PATHLOSS_REFLOSS));
    }
    wifiPhyHelper.SetChannel (wifiChannelHelper.Create ());

    bool qos = false;
    bool ht = false;
    uint32_t channelWidth = WIFI_TOPO_CHANNEL_WIDTH;
    // Set guard interval, qos and ht for 802.11 and ac
    if ( standard == WIFI_STANDARD_80211n
        || standard == WIFI_STANDARD_80211ac
       ) {
        wifiPhyHelper.Set ("ShortGuardEnabled", BooleanValue (false));
        // Set MIMO capabilities for high rate, see YansWifiPhy::ConfigureHtDeviceMcsSet
        wifiPhyHelper.Set ("TxAntennas", UintegerValue (8));
        wifiPhyHelper.Set ("RxAntennas", UintegerValue (8));

        qos = ht = true;
    }

    wifiPhyHelper.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

    //Uncomment the line below to ease troubleshooting
    //wifiPhyHelper.EnablePcapAll ("WifiTopo-wifi");

    // Create wireless channel, see wifi-phy-standard.h
    m_wifi.SetStandard (standard);

    // See wifi-phy.h
    if (rateMode.GetUid () == 0) {
        m_wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");
    } else {
        m_wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode", StringValue (rateMode.GetUniqueName ()),
                                        "ControlMode", StringValue (rateMode.GetUniqueName ()));
    }

    // Uncomment the line below for debugging purposes
    // EnableWifiLogComponents ();

    Packet::EnablePrinting ();

    // Install phy and mac
    Ssid ssid = Ssid ("ns-3-ssid");

    WifiMacHelper wifiMacHelper;

    // Set up station
    wifiMacHelper.SetType ("ns3::StaWifiMac",
                           "Ssid", SsidValue (ssid),
                           "ActiveProbing", BooleanValue (false),
                           "QosSupported", BooleanValue (qos),
                           "HtSupported", BooleanValue (ht));
    m_staDevices = m_wifi.Install (wifiPhyHelper, wifiMacHelper, m_wifiStaNodes);

    // Set up AP
    wifiMacHelper.SetType ("ns3::ApWifiMac",
                           "Ssid", SsidValue (ssid),
                           "QosSupported", BooleanValue (qos),
                           "HtSupported", BooleanValue (ht));
    m_apDevices = m_wifi.Install (wifiPhyHelper, wifiMacHelper, m_wifiApNode);

    // Set channel width
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth",
                 UintegerValue (channelWidth));

    // Configure mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    for (uint32_t i = 0; i < nWifi; ++i) {
        positionAlloc->Add (Vector (1.0, 0.0, 0.0));
    }
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (m_wifiStaNodes);
    mobility.Install (m_wifiApNode);

    // install protocol stack
    InternetStackHelper stack;
    stack.Install (m_wifiStaNodes);
    stack.Install (m_wiredNodes);

    Ipv4AddressHelper address;

    address.SetBase ("10.1.1.0", "255.255.255.0");
    address.Assign (m_wiredDevices);

    address.SetBase ("10.1.2.0", "255.255.255.0");
    address.Assign (m_staDevices);
    address.Assign (m_apDevices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Disable tc now, some bug in ns3 cause extra delay
    TrafficControlHelper tch;
    tch.Uninstall (m_wiredDevices);
    tch.Uninstall (m_staDevices);
    tch.Uninstall (m_apDevices);
}

ApplicationContainer WebrtcWifiTopo::InstallTCP (const std::string& flowId,
                                           uint32_t nodeId,
                                           uint16_t serverPort,
                                           bool downstream)
{
    NS_ASSERT (nodeId <  m_wifiStaNodes.GetN ());

    auto sender = m_wifiStaNodes.Get (nodeId);
    auto receiver = m_wiredNodes.Get (1);

    if (downstream) {
        std::swap (sender, receiver);
    }

    return WebrtcTopo::InstallTCP (flowId,
                             sender,
                             receiver,
                             serverPort);
}

ApplicationContainer WebrtcWifiTopo::InstallCBR (uint32_t nodeId,
                                           uint16_t serverPort,
                                           uint64_t bitrate,
                                           uint32_t packetSize,
                                           bool downstream)
{
    NS_ASSERT (nodeId <  m_wifiStaNodes.GetN ());

    auto sender = m_wifiStaNodes.Get (nodeId);
    auto receiver = m_wiredNodes.Get (1);
    if (downstream) {
        std::swap (sender, receiver);
    }

    return WebrtcTopo::InstallCBR (sender,
                             receiver,
                             serverPort,
                             bitrate,
                             packetSize);
}


ApplicationContainer WebrtcWifiTopo::InstallWebrtc (const std::string& flowId,
                                             uint32_t nodeId,
                                             uint16_t serverPort,
                                             bool downstream,
                                             double initBw,
                                             double minBw,
                                             double maxBw)
{
    NS_ASSERT (nodeId <  m_wifiStaNodes.GetN ());

    auto sender = m_wifiStaNodes.Get (nodeId);
    auto receiver = m_wiredNodes.Get (1);
    if (downstream) {
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

Vector WebrtcWifiTopo::GetPosition (uint32_t idx) const
{
    auto node = m_wifiStaNodes.Get (idx);
    auto mobility = node->GetObject<MobilityModel> ();
    return mobility->GetPosition ();
}

void WebrtcWifiTopo::SetPosition (uint32_t idx, const Vector& position)
{
    auto node = m_wifiStaNodes.Get (idx);
    auto mobility = node->GetObject<MobilityModel> ();
    mobility->SetPosition (position);
}

void WebrtcWifiTopo::EnableWifiLogComponents ()
{
    LogLevel l = (LogLevel)(LOG_LEVEL_ALL|LOG_PREFIX_TIME|LOG_PREFIX_NODE);

    const char* components[] = {
            // For debugging packet loss
            "UdpSocketImpl",
            "Ipv4L3Protocol",
            "Ipv4Interface",
            "ArpL3Protocol",
            "ArpCache",
            // Wifi-related
            "WifiNetDevice",
            "Aarfcd",
            "AdhocWifiMac",
            "AmrrWifiRemoteStation",
            "ApWifiMac",
            "ArfWifiManager",
            "Cara",
            "DcaTxop",
            "DcfManager",
            "DsssErrorRateModel",
            "EdcaTxopN",
            "InterferenceHelper",
            "Jakes",
            "MacLow",
            "MacRxMiddle",
            "MsduAggregator",
            "MsduStandardAggregator",
            "NistErrorRateModel",
            "OnoeWifiRemoteStation",
            "PropagationLossModel",
            "RegularWifiMac",
            "RraaWifiManager",
            "StaWifiMac",
            "SupportedRates",
            "WifiChannel",
            "WifiPhyStateHelper",
            "WifiPhy",
            "WifiRemoteStationManager",
            "YansErrorRateModel",
            "YansWifiChannel",
            "YansWifiPhy",
        };

    for (size_t i = 0; i < sizeof (components) / sizeof (components[0]); i++) {
        LogComponentEnable (components[i], l);
    }
}

}

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
 * Sender application implementation for rmcat ns3 module.
 *
 * @version 0.1.1
 * @author Jiantao Fu
 * @author Sergio Mena
 * @author Xiaoqing Zhu
 */

#include "webrtc-sender.h"
#include "ns3/goog_cc_network_controller.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/video_header.h"
#include "ns3/msg_header.h"
#include <sys/stat.h>
#include "unistd.h"

NS_LOG_COMPONENT_DEFINE ("WebrtcSender");
WebrtcSender::WebrtcSender ()
: m_destIP{}
, m_destPort{0}
, m_initBw{0}
, m_minBw{0}
, m_maxBw{0}
, m_paused{false}
, m_sequence{0}
, m_socket{NULL}
, m_enqueueEvent{}
, m_sendEvent{}
, m_fps{30.}
, m_rVin{0.}
, m_rSend{0.}
{}

WebrtcSender::~WebrtcSender () {}

void WebrtcSender::PauseResume (bool pause)
{
    NS_ASSERT (pause != m_paused);
    if (pause) {
        Simulator::Cancel (m_enqueueEvent);   
    } else {
        m_rVin = m_initBw;
        m_rSend = m_initBw;
        m_enqueueEvent = Simulator::ScheduleNow (&WebrtcSender::GenerateVideoFrame, this);        
    }
    m_paused = pause;
}

void WebrtcSender::SetCodec (std::shared_ptr<syncodecs::Codec> codec)
{
    m_codec = codec;
}

// TODO (deferred): allow flexible input of video traffic trace path via config file, etc.
//注意这里和rmcat不同，编码器输出的是frame size
void WebrtcSender::SetCodecType (SyncodecType codecType)
{
    syncodecs::Codec* codec = NULL;
    switch (codecType) {
        case SYNCODEC_TYPE_PERFECT:
        {
            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
            break;
        }
        case SYNCODEC_TYPE_FIXFPS:
        {
            m_fps = SYNCODEC_DEFAULT_FPS;
            codec = new syncodecs::SimpleFpsBasedCodec{m_fps};            
            break;
        }
        case SYNCODEC_TYPE_STATS:
        {
            m_fps = SYNCODEC_DEFAULT_FPS;
            codec = new syncodecs::StatisticsCodec{m_fps};            
            break;
        }
        case SYNCODEC_TYPE_TRACE:
        case SYNCODEC_TYPE_HYBRID:
        {
            const std::vector<std::string> candidatePaths = {
                ".",      // If run from top directory (e.g., with gdb), from ns-3.26/
                "..",    // If run from with test_new.py with designated directory, from ns-3.26/2017-xyz/
                "../..",  // If run with test.py, from ns-3.26/testpy-output/201...
            };

            /* const std::string traceSubDir{"model/apps/syncodecs/video_traces/chat_firefox_h264"};
            std::string traceDir{}; 
            char path[256];
            getcwd(path,256);    
            std::string dir(path);   
            for (auto c : candidatePaths) {
                std::ostringstream currPathOss;
                currPathOss << c << "/" << traceSubDir;
                struct stat buffer;                
                
                if (::stat (currPathOss.str ().c_str (), &buffer) == 0) {
                    //filename exists
                    traceDir = currPathOss.str ();
                    break;
                }
            }

            NS_ASSERT_MSG (!traceDir.empty (), "Traces file not found in candidate paths"); */
             
            const std::string traceDir{"/home/ns/workspace/ns-allinone-3.39/ns-3.39/src/webrtc/model/apps/syncodecs/video_traces/chat_firefox_h264"};
            auto filePrefix = "chat";
            codec = (codecType == SYNCODEC_TYPE_TRACE) ?
                                 new syncodecs::TraceBasedCodecWithScaling{
                                    traceDir,        // path to traces directory
                                    filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true} :          // fixed mode: image resolution doesn't change
                                 new syncodecs::HybridCodec{
                                    traceDir,        // path to traces directory
                                    filePrefix,      // video filename
                                    SYNCODEC_DEFAULT_FPS,             // Default FPS: 30fps
                                    true};           // fixed mode: image resolution doesn't change
	        m_fps = SYNCODEC_DEFAULT_FPS;            
            break;
        }
        case SYNCODEC_TYPE_SHARING:
        {
            codec = new syncodecs::SimpleContentSharingCodec{};           
            break;
        }
        default:  // defaults to perfect codec
            codec = new syncodecs::PerfectCodec{DEFAULT_PACKET_SIZE};
    }

    // update member variable
    m_codec = std::shared_ptr<syncodecs::Codec>{codec};
}

void WebrtcSender::SetController (std::unique_ptr<NetworkControllerInterface> controller)
{
    m_controller = std::move(controller);
}

void WebrtcSender::Setup (Ipv4Address destIP,
                         uint16_t destPort, 
                         double initBw,
                         double minBw,
                         double maxBw)
{
    if (!m_codec) {
        SetCodecType (SyncodecType::SYNCODEC_TYPE_TRACE);
    }    

    m_destIP = destIP;
    m_destPort = destPort;
    m_initBw = initBw;
    m_minBw = minBw;
    m_maxBw = maxBw;
}

void WebrtcSender::SetRinit (float r)
{
    m_initBw = r;
    //if (m_controller) m_controller->setInitBw (m_initBw);
}

void WebrtcSender::SetRmin (float r)
{
    m_minBw = r;
    //if (m_controller) m_controller->setMinBw (m_minBw);
}

void WebrtcSender::SetRmax (float r)
{
    m_maxBw = r;
    //if (m_controller) m_controller->setMaxBw (m_maxBw);
}

void WebrtcSender::StartApplication ()
{     
    m_sequence = rand ();
    NS_ASSERT (m_minBw <= m_initBw);
    NS_ASSERT (m_initBw <= m_maxBw);

    m_rVin = m_initBw;
    m_rSend = m_initBw;

    if (m_socket == NULL) {
        m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto res = m_socket->Bind ();
        NS_ASSERT (res == 0);
    }
       

    if (!m_controller) {
      TargetRateConstraints constaints;
      constaints.at_time = Timestamp::Millis(Simulator::Now().GetMilliSeconds());
      constaints.starting_rate = RtcDataRate::BitsPerSec(m_initBw);
      constaints.min_data_rate = RtcDataRate::BitsPerSec(m_minBw);
      constaints.max_data_rate = RtcDataRate::BitsPerSec(m_maxBw);
        m_controller = std::make_unique<GoogCcNetworkController> (constaints);
    } else {
        m_controller.reset ();
    }

    if(m_transportController == nullptr){
        m_udpSocket.SetSocket(m_socket);
        m_udpSocket.SetIp(m_destIP);
        m_udpSocket.SetPort(m_destPort);
        m_transportController = std::make_unique<TransportController>(&m_udpSocket, m_controller.get());
        m_socket->SetRecvCallback (MakeCallback (&TransportController::RecvPacket, m_transportController.get())); 
        m_transportController->UpdatePacingRate(RtcDataRate::BitsPerSec(m_rVin));
    }

    m_enqueueEvent = Simulator::Schedule (Seconds (0.0), &WebrtcSender::GenerateVideoFrame, this);   
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &TransportController::Start, m_transportController.get()); 
}

void WebrtcSender::StopApplication ()
{
    Simulator::Cancel (m_enqueueEvent);
    Simulator::Cancel (m_sendEvent);      
}

//注意，对于webrtc sender来说，不能使用packetizer codec, 应该取出的是frame，而不是packet
void WebrtcSender::GenerateVideoFrame ()
{
    syncodecs::Codec& codec = *m_codec;
    codec.setTargetRate (m_rVin);
    ++codec; // Advance codec/packetizer to next frame/packet
    const auto bytesToSend = codec->first.size ();
    NS_ASSERT (bytesToSend > 0);    
    
    double secsToNextEnqPacket = codec->second;
    Time tNext{Seconds (secsToNextEnqPacket)};
    m_enqueueEvent = Simulator::Schedule (tNext, &WebrtcSender::GenerateVideoFrame, this);

    SendVideoFrame(bytesToSend);    
}

//应该是进入PacketQueue（Pacer里面）
//直接调用PacedSender.EnqueuePacket
void WebrtcSender::SendVideoFrame (uint32_t frameSize) {
    NS_ASSERT(frameSize > 0);
    uint16_t packet_cnt = (frameSize + DEFAULT_PACKET_SIZE - 1) / DEFAULT_PACKET_SIZE;
    uint32_t remain_size = frameSize - (packet_cnt - 1) * DEFAULT_PACKET_SIZE;
    uint16_t first_seq = 0;
    for (int i = 0; i < packet_cnt; i++) {     
        if (i == 0) {            
            first_seq = m_sequence;
        }        
        uint32_t payload_size = (i == packet_cnt - 1? remain_size : DEFAULT_PACKET_SIZE);

        VideoHeader video_header{m_sequence, m_frameId, first_seq, packet_cnt, static_cast<uint16_t>(payload_size)};
        auto packet = Create<Packet>(payload_size);
        packet->AddHeader(video_header);

        MsgHeader msg_header{PacketType::kVideo, static_cast<uint16_t>(video_header.GetSerializedSize())};
        packet->AddHeader(msg_header);

        std::unique_ptr<PacketToSend> packet_to_send(new PacketToSend());
        packet_to_send->packet_type = PacketType::kVideo;
        packet_to_send->payload_size = payload_size;
        packet_to_send->packet = packet;
        m_transportController->EnqueuePacket(std::move(packet_to_send));
        m_sequence++;
    }
    m_frameId++;   
}

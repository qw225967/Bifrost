#ifndef WEBRTC_SENDER_H
#define WEBRTC_SENDER_H

#include "ns3/syncodecs.h"
#include "ns3/socket.h"
#include "ns3/application.h"
#include "ns3/webrtc-constants.h"
#include "ns3/common.h"
#include <memory>
#include <deque>
#include "ns3/transport_controller.h"
#include "ns3/webrtc_udp_socket.h"

using namespace ns3;
using namespace syncodecs;
class WebrtcSender : public Application {
  public:
    WebrtcSender();
    virtual ~WebrtcSender();

    void PauseResume(bool pause);
    void SetCodec(std::shared_ptr<Codec> codec);
    void SetCodecType(SyncodecType codecType);

    void SetController(std::unique_ptr<NetworkControllerInterface> controller);

    void SetRinit(float Rinit);
    void SetRmin(float Rmin);
    void SetRmax(float Rmax);

    void Setup(Ipv4Address dest_ip, uint16_t dest_port, double initBw, double minBw, double maxBw);

  private:
    virtual void StartApplication();
    virtual void StopApplication();

    void GenerateVideoFrame();
    void SendVideoFrame (uint32_t frameSize);    
    void RecvPacket (Ptr<Socket> socket);
    

    std::shared_ptr<Codec> m_codec;
    std::unique_ptr<NetworkControllerInterface> m_controller;
    std::unique_ptr<TransportController> m_transportController;
    Ipv4Address m_destIP;
    uint16_t m_destPort;
    float m_initBw;
    float m_minBw;
    float m_maxBw;
    bool m_paused;
    
    uint16_t m_sequence; 
    uint16_t m_frameId{1};   
    Ptr<Socket> m_socket;
    EventId m_enqueueEvent;
    EventId m_sendEvent;

    float m_fps;  // frames-per-second
    double m_rVin; //bps
    double m_rSend; //bps  

    WebrtcUdpSocket m_udpSocket;

    //考虑kFec, kRetransmission, kVideo的区分问题,解析的时候根据RemoveHeader调用相应Header的反序列化
    //所以必须能够区分类型才可以（Rtcpheader就可以区分类型）
    //根据RTP 中的 payload type来进行分类可以（或者根据ssrc来进行区分）
    //所以可以按照这种层层剥离的方式来进行，发送端处理RTCP packets, 接收端处理RTP packets
    std::map<uint64_t, std::unique_ptr<ns3::Packet>> rtp_packet_history;
};

#endif
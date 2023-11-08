#ifndef WEBRTC_RECEIVER_H
#define WEBRTC_RECEIVER_H

#include "ns3/socket.h"
#include "ns3/video_header.h"
#include "ns3/msg_header.h"
#include "ns3/ack_header.h"
#include "ns3/fec_header.h"
#include "ns3/echo_header.h"
#include "ns3/application.h"
#include "ns3/nack_requester.h"

class WebrtcReceiver : public Application, public NackSender {
  public:
    WebrtcReceiver();
    ~WebrtcReceiver();

    void Setup(uint16_t port);
  private:
    virtual void StartApplication();
    virtual void StopApplication();

    void RecvPacket(Ptr<Socket> socket);
    void OnVideoPacket(VideoHeader& header, uint64_t now_ms);
    void OnFecPacket(FecHeader& header, uint64_t now_ms);
    void OnEchoReply(EchoHeader& header, uint64_t now_ms);
    void AddFeedback(uint16_t, uint64_t recv_time);
    void SendAckFeedback ();
    void SendEchoRequest();

    //override nacksender
    void SendNack(std::vector<uint16_t>& sequence_numbers, bool buffering_allowed) override;
    
    bool m_running;
    bool m_waiting;    
    Ipv4Address m_srcIp;
    uint16_t m_srcPort;
    Ptr<Socket> m_socket;
    AckHeader m_ackHeader;
    EchoHeader m_echoHeader;
    EventId m_sendFeedbackEvent;
    EventId m_sendEchoEvent;
    uint64_t m_periodUs;
    uint64_t m_echoPeriodMs;

    std::unique_ptr<NackRequester> nack_requester_;
    bool enable_nack_;
    bool enable_fec_;
};

#endif
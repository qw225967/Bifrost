#include "webrtc-receiver.h"
#include "webrtc-constants.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/udp-socket-factory.h"

WebrtcReceiver::WebrtcReceiver()
: m_running{false},
  m_waiting{false},
  m_srcIp{},
  m_srcPort{},
  m_socket{NULL},
  m_ackHeader{},
  m_sendFeedbackEvent{},
  m_sendEchoEvent{},
  m_periodUs{WEBRTC_FEEDBACK_PERIOD_US},
  m_echoPeriodMs{WEBRTC_ECHO_PERIOD_MS}
 {
    nack_requester_ = std::make_unique<NackRequester>(this);
 }

 WebrtcReceiver::~WebrtcReceiver() {}

 void WebrtcReceiver::Setup(uint16_t port){
    m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
    auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
    auto ret = m_socket->Bind (local);
    NS_ASSERT (ret == 0);
    m_socket->SetRecvCallback (MakeCallback (&WebrtcReceiver::RecvPacket, this));

    m_running = false;
    m_waiting = true;
 }

 void WebrtcReceiver::StartApplication ()
{
    m_running = true;      
    Time tFirst {MicroSeconds (m_periodUs)};
    m_sendFeedbackEvent = Simulator::Schedule (tFirst, &WebrtcReceiver::SendAckFeedback, this);
    m_sendEchoEvent = Simulator::Schedule(MilliSeconds(m_echoPeriodMs), &WebrtcReceiver::SendEchoRequest, this);
}

void WebrtcReceiver::StopApplication ()
{
    m_running = false;
    m_waiting = true;    
    Simulator::Cancel (m_sendFeedbackEvent);
    Simulator::Cancel(m_sendEchoEvent);
}

void WebrtcReceiver::RecvPacket(Ptr<Socket> socket){
  if(!m_running){
    return;
  }
  Address remoteAddr{};
  auto packet = m_socket->RecvFrom(remoteAddr);
  m_srcIp = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
  m_srcPort = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();
  NS_ASSERT(packet);
  MsgHeader msg_header{};
  packet->RemoveHeader(msg_header);
  if(msg_header.Type() == PacketType::kVideo){
    VideoHeader video_header{};
    packet->RemoveHeader(video_header, msg_header.Size());
    OnVideoPacket(video_header, Simulator::Now().GetMilliSeconds());
  }
  if(msg_header.Type() == PacketType::kForwardErrorCorrection){
    FecHeader fec_header{};
    packet->RemoveHeader(fec_header, msg_header.Size());
    OnFecPacket(fec_header, Simulator::Now().GetMilliSeconds());
  }
  if(msg_header.Type() == PacketType::kEchoReply){
    EchoHeader echo_header{};
    packet->RemoveHeader(echo_header, msg_header.Size());
    OnEchoReply(echo_header, Simulator::Now().GetMilliSeconds());    
  }
}

void WebrtcReceiver::OnVideoPacket(VideoHeader& header, uint64_t now_ms){
  AddFeedback(header.Seq(), now_ms);
}

void WebrtcReceiver::OnFecPacket(FecHeader& header, uint64_t now_ms){

}

void WebrtcReceiver::OnEchoReply(EchoHeader& header, uint64_t now_ms){
  if(header.Seq() != m_echoHeader.Seq()){
    return;
  }
  nack_requester_->OnRtt(now_ms - header.Ts(), now_ms);
  m_echoHeader.IncreaseEchoSeq();
}

void WebrtcReceiver::SendAckFeedback(){
  if(m_running && !m_ackHeader.Packets().empty()){
     auto packet = Create<Packet>();
     packet->AddHeader(m_ackHeader);
     auto header_size = static_cast<uint16_t>(m_ackHeader.GetSerializedSize());
     MsgHeader msg_header{};
     msg_header.SetType(PacketType::kAck);
     msg_header.SetSize(header_size);
     packet->AddHeader(msg_header);
     m_socket->SendTo(packet, 0, InetSocketAddress{m_srcIp, m_srcPort});
     m_ackHeader.Clear();
     m_ackHeader.IncreaseAckSeq();
  }
  m_sendFeedbackEvent = Simulator::Schedule(MicroSeconds(m_periodUs), &WebrtcReceiver::SendAckFeedback, this);
}

void WebrtcReceiver::SendEchoRequest() {
  if(m_running){
    auto packet = Create<Packet>();
    m_echoHeader.SetTimestamp(Simulator::Now().GetMilliSeconds());
    packet->AddHeader(m_echoHeader);
    MsgHeader msg_header{PacketType::kEchoRequest, static_cast<uint16_t>(m_echoHeader.GetSerializedSize())};
    packet->AddHeader(msg_header);
    m_socket->SendTo(packet, 0, InetSocketAddress{m_srcIp, m_srcPort});    
  }
  m_sendEchoEvent = Simulator::Schedule(MilliSeconds(m_echoPeriodMs), &WebrtcReceiver::SendEchoRequest, this);
}

void WebrtcReceiver::AddFeedback(uint16_t seq, uint64_t recv_time){
  m_ackHeader.OnVideoPacket(seq, recv_time);
}

void WebrtcReceiver::SendNack(std::vector<uint16_t>& sequence_numbers, bool buffering_allowed) {
  
}


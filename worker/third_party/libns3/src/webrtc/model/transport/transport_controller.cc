#include "transport_controller.h"
#include "ns3/goog_cc_network_controller.h"
#include "ns3/core-module.h"
#include "ns3/msg_header.h"
#include "ns3/ack_header.h"
#include "ns3/nack_header.h"
#include "ns3/echo_header.h"
#include "ns3/packet.h"
#include <assert.h>
TransportController::TransportController(WebrtcUdpSocket* udp_socket, NetworkControllerInterface* controller)
	:udp_socket_(udp_socket),    
	network_controller_(controller)	 {	
	transport_feedback_adapter_ = std::make_unique<TransportFeedbackAdapter>();
	packet_router_ = std::make_unique<PacketRouter>(udp_socket, this);
	paced_sender_ = std::make_unique<PacedSender>(packet_router_.get());
}

TransportController::~TransportController() {
    printf("failed!");
}

void TransportController::EnqueuePacket(std::unique_ptr<PacketToSend> packet){
	paced_sender_->EnqueuePacket(std::move(packet));
}

void TransportController::OnSentPacket(uint64_t seq, uint64_t send_time) {
	std::optional<SentPacket> packet_msg;
	{
		//std::lock_guard lock(mutex_);
		packet_msg = transport_feedback_adapter_->ProcessSentPacket(
		seq, Timestamp::Millis(int64_t(send_time)));
	}
	if (network_controller_ && packet_msg) {
		network_controller_->OnSentPacket(*packet_msg);
	}
}

//rtp sender egress的SendPacket并不是直接通过socket发送出去了，所以会在adapter里面预留一个packet item
//等真实的发送之后，上面的OnSentPacket会更新packet中的相关信息（具体的发送时间）
void TransportController::OnAddPacket(const VideoPacketSendInfo& packet_info) {	
	Timestamp creation_time = Timestamp::Millis(Simulator::Now().GetMilliSeconds());	
	transport_feedback_adapter_->AddPacket(packet_info, 0, creation_time);
}

//关于pace的情况 等会再添加
void TransportController::OnTransportFeedback(std::map<uint16_t, uint64_t>& feedback) {
	auto feedback_time = Timestamp::Millis(Simulator::Now().GetMilliSeconds());
    std::optional<TransportPacketsFeedback> feedback_msg;
    feedback_msg = transport_feedback_adapter_->ProcessTransportFeedback(feedback, feedback_time);
    if (feedback_msg)
    {
        if (network_controller_)
        {
            network_controller_->OnTransportPacketsFeedback(*feedback_msg);
        }
    }
}

void TransportController::UpdatePacingRate(RtcDataRate pacing_rate) {
	assert(pacing_rate != RtcDataRate::Zero());
	paced_sender_->SetPacingRates(pacing_rate, RtcDataRate::Zero());
}

void TransportController::Start() {
	Simulator::Schedule(Seconds(0.0), &PacedSender::Start, paced_sender_.get()); 
}

void TransportController::RecvPacket (Ptr<Socket> socket)
{
    Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    NS_ASSERT (packet);

    /* auto rIPAddress = InetSocketAddress::ConvertFrom (remoteAddr).GetIpv4 ();
    auto rport = InetSocketAddress::ConvertFrom (remoteAddr).GetPort ();

		assert (rIPAddress == m_destIP);
    assert (rport == m_destPort);
   */
    // get the feedback header
    //const uint64_t nowUs = Simulator::Now ().GetMicroSeconds ();
    MsgHeader msg_header{};
    packet->RemoveHeader(msg_header);
    if (msg_header.Type() == PacketType::kAck)
    {
        AckHeader ack_header{};
        packet->RemoveHeader(ack_header, msg_header.Size());
        OnTransportFeedback(ack_header.Packets());
    }
    if (msg_header.Type() == PacketType::kNack)
    {
        NackHeader nack_header{};
        packet->RemoveHeader(nack_header, msg_header.Size());
    }
    if (msg_header.Type() == PacketType::kEchoRequest)
    {
        EchoHeader echo_header{};
        packet->RemoveHeader(echo_header, msg_header.Size());
        msg_header.SetType(PacketType::kEchoReply);
        auto reply_packet = Create<Packet>();
        reply_packet->AddHeader(echo_header);
        reply_packet->AddHeader(msg_header);
        udp_socket_->Send(reply_packet);
    }
}
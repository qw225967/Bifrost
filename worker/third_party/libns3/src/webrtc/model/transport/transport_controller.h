//其功能类似于webrtc中的RtpTransportControlSend
#ifndef TRANSPORT_CONTROLLER_H
#define TRANSPORT_CONTROLLER_H

#include <stdint.h>
#include <memory>
#include <mutex>
#include "ns3/common.h"
#include "transport_feedback_adapter.h"
#include "ns3/ack_header.h"
#include "ns3/paced_sender.h"
#include "ns3/packet_router.h"
#include "ns3/socket.h"
#include "ns3/webrtc_udp_socket.h"

using namespace rtc;
class TransportController {
public:
	TransportController(WebrtcUdpSocket* udp_socket, NetworkControllerInterface* controller);
	~TransportController();
	void EnqueuePacket(std::unique_ptr<PacketToSend> packet);
	void OnSentPacket(uint64_t seq, uint64_t send_time);
	void OnAddPacket(const VideoPacketSendInfo& packet_info);
	void OnTransportFeedback(std::map<uint16_t, uint64_t>& feedback);

	PacedSender* paced_sender() { return paced_sender_.get(); }
	void UpdatePacingRate(RtcDataRate pacing_rate);
	void Start();	
	void RecvPacket (Ptr<Socket> socket);
private:	
	WebrtcUdpSocket* udp_socket_;
	std::unique_ptr<TransportFeedbackAdapter> transport_feedback_adapter_;
	NetworkControllerInterface* network_controller_;
	std::unique_ptr<PacedSender> paced_sender_;
	std::unique_ptr<PacketRouter> packet_router_;
};
#endif
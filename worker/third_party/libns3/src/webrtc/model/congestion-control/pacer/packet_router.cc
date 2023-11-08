#include "packet_router.h"
#include "ns3/transport_controller.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"

PacketRouter::PacketRouter(WebrtcUdpSocket* udp_socket, TransportController* controller)
	:transport_seq_(1),	
	udp_socket_(udp_socket), 
	transport_controller_(controller)	
{}

PacketRouter::~PacketRouter() {}

//暂时不考虑探测的问题，paced packet info暂时先不管
void PacketRouter::SendPacket(std::unique_ptr<PacketToSend> packet,
	const PacedPacketInfo& cluster_info) {
	//在webrtc中，会调用不同的rtp rtcp module来进行发送
	//但是在gazer中，则通过udp socket来发送即可
	//zai
	uint64_t seq = packet->seq;	
	if (transport_controller_) {
		transport_controller_->OnSentPacket(seq, Simulator::Now().GetMilliSeconds());
	}
	udp_socket_->Send(packet->packet);	
}

std::vector<std::unique_ptr<PacketToSend>> PacketRouter::FetchFec() {
	std::vector<std::unique_ptr<PacketToSend>> temp;
	return temp;
}
std::vector<std::unique_ptr<PacketToSend>> PacketRouter::GeneratePadding(DataSize size) {
	std::vector<std::unique_ptr<PacketToSend>> temp;
	return temp;
}

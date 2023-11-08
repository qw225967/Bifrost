#pragma once
#include <stdint.h>
#include <memory>
#include "ns3/common.h"
#include "ns3/data_size.h"
#include "ns3/webrtc_udp_socket.h"

using namespace ns3;
class TransportController;
class PacketRouter {
public:
	PacketRouter(WebrtcUdpSocket* udp_socket, TransportController* controller);	
	~PacketRouter();

	void SendPacket(std::unique_ptr<PacketToSend> packet,
		const PacedPacketInfo& cluster_info);

	std::vector<std::unique_ptr<PacketToSend>> FetchFec();
	std::vector<std::unique_ptr<PacketToSend>> GeneratePadding(DataSize size);

private:
	uint16_t transport_seq_;
	WebrtcUdpSocket* udp_socket_;	
	TransportController* transport_controller_;
};
#include "webrtc_udp_socket.h"


WebrtcUdpSocket::WebrtcUdpSocket() {}
WebrtcUdpSocket::~WebrtcUdpSocket(){}

void WebrtcUdpSocket::Send(Ptr<Packet> packet){
  m_socket->SendTo(packet, 0, InetSocketAddress{m_destIp, m_destPort});
}

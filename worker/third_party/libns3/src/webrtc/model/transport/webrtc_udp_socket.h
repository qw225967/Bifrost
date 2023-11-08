#ifndef WEBRTC_UDP_SOCKET_H
#define WEBRTC_UDP_SOCKET_H

#include "ns3/socket.h"

using namespace ns3;

class WebrtcUdpSocket {
  public:
    WebrtcUdpSocket();
    ~WebrtcUdpSocket();

    void SetSocket(Ptr<Socket> socket) { m_socket = socket;}
    void SetIp(Ipv4Address ip) { m_destIp = ip;}
    void SetPort(uint16_t port) { m_destPort = port;}

    void Send(Ptr<Packet> packet);
  private:
    Ptr<Socket> m_socket;
    Ipv4Address m_destIp;
    uint16_t m_destPort;
};

#endif
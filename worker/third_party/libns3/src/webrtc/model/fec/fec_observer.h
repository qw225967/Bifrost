#pragma once
#include "ns3/fec_header.h"
class FecObserver {
public:
	virtual void OnFecPacket(FecHeader fec_packet) = 0;
};

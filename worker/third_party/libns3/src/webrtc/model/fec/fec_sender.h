#pragma once
#include <stdint.h>
#include <stddef.h>
class FecSender {
public:
	virtual void SendFecPacket(uint8_t* buffer, size_t size) = 0;
};
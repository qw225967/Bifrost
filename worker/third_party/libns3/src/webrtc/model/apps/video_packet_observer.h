#pragma once
#include "ns3/video_header.h"
class VideoPacketObserver {
public:
	virtual void OnVideoPacket(VideoHeader packet, uint64_t now_ms) = 0;
};
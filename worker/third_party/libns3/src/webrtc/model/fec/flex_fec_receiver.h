#pragma once
#include <set>
#include "ns3/fec_header.h"
#include "ns3/video_packet_assembler.h"

#include "ns3/ack_header.h"
#include "ns3/fec_observer.h"
#include <mutex>
#include <thread>
#include <deque>

//基于三级联动机制的码率分配优化方案  基础级：IQA ->NACK->FEC（FEC是可选的）
//NACK(高延时，低带宽）
//FEC(低延时，高带宽）
//所以要考虑：NACK-LOSS  FEC-LOSS  NACK+FEC->LOSS
//要设置target delay（必须要设置这个）
//target IQA(决定了基础码率）[推算延迟计算（I帧，P帧）的大小等，编码解码渲染等]
//target delay - basic delay（决定了
class  FlexFecReceiver : public FecObserver {
public:
	FlexFecReceiver(VideoPacketAssembler*, AckHeader* ack_header);
	~FlexFecReceiver();

	void OnFecPacket(FecHeader packet) override;
	void OnVideoPacket(uint16_t frame_id);
private:
	//由于不进行载荷判定，需要每个video packet承载第一个packet seq和最后一个packet seq,这样即使首包和末包丢失也没有关系
	void FecDecode(uint16_t frame_id, uint16_t fec_id, std::vector<uint16_t> protected_packets);
	void FecDecodeAll(uint16_t frame_id);
	void ProcessFecPacket();
	

	VideoPacketAssembler* video_packet_assembler_;
	AckHeader* ack_header_;
	//std::vector<VideoPacketObserver*> video_packet_observers_;

	//保存protected packets，因为需要反复来进行FEC验证
	//每次收到一个该frame id的packet就要进行校验
	//如果一个fec packet对应的已经恢复好，则删除之
	//frame_id索引 + fec_id索引
	std::map<uint16_t, std::map<uint16_t, std::vector<uint16_t>>> fec_packets_;

	std::thread fec_process_thread_;
	std::mutex fec_process_mutex_;
	std::deque<FecHeader> fec_packet_queue_;
};
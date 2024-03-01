/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/28 10:19 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : MixError.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_MIXERROR_H
#define WORKER_MIXERROR_H

#include <vector>
#include "ns3/core-module.h"
#include "ns3/error-model.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

class MixError : public BurstErrorModel {
public:
	MixError();

	static TypeId GetTypeId(void);
	// 第一列，时间(s)；第二列，连续丢包数；第三列，队列突变；第四列，竞争流
	void SetRecordItem(std::vector<uint32_t>);

	void CreatUpLinkUser(Ptr<Node> a);
	void UserUplinkCongestion(uint32_t n, uint32_t stop_time, Ptr<Node> a);

private:
	bool DoCorrupt (Ptr<Packet> p);

	std::vector<uint32_t> item_;
	uint32_t lost_count_ = 0;
	bool enable_ = false;
};

#endif // WORKER_MIXERROR_H

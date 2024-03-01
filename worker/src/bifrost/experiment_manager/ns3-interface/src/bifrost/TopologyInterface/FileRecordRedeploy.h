/*******************************************************
 * @author      : dog head
 * @date        : Created in 2024/2/27 2:13 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : FileRecordRecurrent.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_FILERECORDREDEPLOY_H
#define WORKER_FILERECORDREDEPLOY_H

#include "TopologyInterface.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

namespace ns3 {
	class FileRecordRedeploy: public TopologyInterface,
														public PointToPointHelper {
	public:
		FileRecordRedeploy();
		~FileRecordRedeploy() {}


		NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);

	private:
		void ReadFileToDataVec();

	private:
//		std::ifstream record_file_;
		// 第一列，时间(s)；第二列，连续丢包数；第三列，竞争包个数/s；第四列，竞争时长
		std::vector<std::vector<uint32_t>> record_data_;
	};

} // namespace ns3

#endif // WORKER_FILERECORDREDEPLOY_H

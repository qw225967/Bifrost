/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 10:04 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : data_producer.cpp
 * @description : TODO
 *******************************************************/

#include "data_producer.h"

namespace bifrost {
DataProducer::DataProducer() {
#ifdef USING_LOCAL_FILE_DATA
  data_file_.open(LOCAL_DATA_FILE_PATH_STRING, std::ios::binary);
#endif
}

RtpPacketPtr DataProducer::CreateData() {
  uint8_t data[900];

#ifdef USING_LOCAL_FILE_DATA
  data_file_.read((char*)data, 900);
#endif

  RtpPacketPtr packet = RtpPacket::Parse(data, 900);
  return packet;
}

DataProducer::~DataProducer() {
#ifdef USING_LOCAL_FILE_DATA
  data_file_.close();
#endif
}
}  // namespace bifrost
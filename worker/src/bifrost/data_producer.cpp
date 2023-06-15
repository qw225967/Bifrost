/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/14 10:04 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : data_producer.cpp
 * @description : TODO
 *******************************************************/

#include "data_producer.h"

#include "modules/rtp_rtcp/source/rtp_packet.h"
#include "modules/rtp_rtcp/source/rtp_packet_received.h"

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

  // 使用webrtc中rtp包初始化方式
  webrtc::RtpPacketReceived receive_packet(nullptr);
  receive_packet.SetSequenceNumber(this->sequence_);
  receive_packet.SetPayloadType(101);
  receive_packet.SetSsrc(this->ssrc_);
  memcpy(receive_packet.Buffer().data(), data, 900);
  receive_packet.SetPayloadSize(900);

  RtpPacketPtr packet =
      RtpPacket::Parse(receive_packet.data(), receive_packet.size());
  return packet;
}

DataProducer::~DataProducer() {
#ifdef USING_LOCAL_FILE_DATA
  data_file_.close();
#endif
}
}  // namespace bifrost
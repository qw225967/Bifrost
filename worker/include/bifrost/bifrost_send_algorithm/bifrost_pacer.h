/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 8:23 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_pacer.h
 * @description : TODO
 *******************************************************/

#ifndef _BIFROST_PACER_H
#define _BIFROST_PACER_H

#include "uv_timer.h"
#include "rtp_packet.h"

namespace bifrost {
class BifrostPacer : public UvTimer{
  BifrostPacer();
  ~BifrostPacer();
 public:
  void OnTimer(UvTimer* timer);

 public:
  void PacketReadyToSend(RtpPacketPtr rtp_packet);

  void set_pacing_rate(uint32_t pacing_rate);

 private:
  uint16_t tcc_ext_seq_ = 0;
};
}

#endif  //_BIFROST_PACER_H

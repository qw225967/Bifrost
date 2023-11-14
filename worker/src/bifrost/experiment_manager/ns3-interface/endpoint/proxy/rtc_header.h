/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/11/13 5:11 下午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : rtc_header.h
 * @description : TODO
 *******************************************************/

#ifndef _RTC_HEADER_H
#define _RTC_HEADER_H

//使用小端序
#define MS_LITTLE_ENDIAN 1

namespace ns3proxy {
const size_t RtpHeaderSize{12};

// RTCP 头
struct RtcpHeader {
#if defined(MS_LITTLE_ENDIAN)
  uint8_t count : 5;
  uint8_t padding : 1;
  uint8_t version : 2;
#elif defined(MS_BIG_ENDIAN)
  uint8_t version : 2;
  uint8_t padding : 1;
  uint8_t count : 5;
#endif
  uint8_t packetType : 8;
  uint16_t length : 16;
};

// sr
struct SRHeader {
  uint32_t ssrc;
  uint32_t ntpSec;
  uint32_t ntpFrac;
  uint32_t rtpTs;
  uint32_t packetCount;
  uint32_t octetCount;
};

// rr
struct RRHeader {
  uint32_t ssrc;
  uint32_t fractionLost : 8;
  uint32_t totalLost : 24;
  uint32_t lastSeq;
  uint32_t jitter;
  uint32_t lsr;
  uint32_t dlsr;
};

// fb
struct FBHeader {
  uint32_t sender_ssrc;
  uint32_t media_ssrc;
};

// RTP 头
struct RtpHeader {
#if defined(MS_LITTLE_ENDIAN)
  uint8_t csrcCount : 4;
  uint8_t extension : 1;
  uint8_t padding : 1;
  uint8_t version : 2;
  uint8_t payloadType : 7;
  uint8_t marker : 1;
#elif defined(MS_BIG_ENDIAN)
  uint8_t version : 2;
  uint8_t padding : 1;
  uint8_t extension : 1;
  uint8_t csrcCount : 4;
  uint8_t marker : 1;
  uint8_t payloadType : 7;
#endif
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint32_t ssrc;
};

}

#endif  //_RTC_HEADER_H

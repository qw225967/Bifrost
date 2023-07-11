/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/7/11 11:15 上午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : rtcp_compound_packet.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_RTCP_COMPOUND_PACKET_H
#define WORKER_RTCP_COMPOUND_PACKET_H

#include "common.h"
#include "rtcp_rr.h"
#include "rtcp_sr.h"

namespace bifrost {
class CompoundPacket
    {
     public:
      CompoundPacket() = default;

     public:
      const uint8_t* GetData() const
      {
        return this->header;
      }
      size_t GetSize() const
      {
        return this->size;
      }
      size_t GetSenderReportCount() const
      {
        return this->sender_report_packet_.GetCount();
      }
      size_t GetReceiverReportCount() const
      {
        return this->receiver_report_packet_.GetCount();
      }
      void Dump();
      void AddSenderReport(SenderReport* report);
      void AddReceiverReport(ReceiverReport* report);
      bool HasSenderReport()
      {
        return this->sender_report_packet_.Begin() != this->sender_report_packet_.End();
      }
      void Serialize(uint8_t* data);

     private:
      uint8_t* header{ nullptr };
      size_t size{ 0 };
      SenderReportPacket sender_report_packet_;
      ReceiverReportPacket receiver_report_packet_;
    };
}

#endif  // WORKER_RTCP_COMPOUND_PACKET_H

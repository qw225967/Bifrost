#ifndef NACK_SENDER_H
#define NACK_SENDER_H

#include <vector>
#include <stdint.h>

class NackSender {
  public:
    virtual void SendNack(std::vector<uint16_t>& sequence_numbers, 
                                      bool buffering_allowed) = 0;
  protected:
    virtual ~NackSender() {}
};

#endif
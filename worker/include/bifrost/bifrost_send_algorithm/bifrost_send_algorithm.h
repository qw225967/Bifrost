/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/8/16 10:48 上午
 * @mail        : qw225967@github.com
 * @project     : .
 * @file        : bifrost_send_algorithm.h
 * @description : TODO
 *******************************************************/

#ifndef _BIFROST_SEND_ALGORITHM_H
#define _BIFROST_SEND_ALGORITHM_H

#include <memory>

#include "bifrost_send_algorithm_interface.h"

namespace bifrost {
typedef std::shared_ptr<BifrostSendAlgorithmInterface>
    BifrostSendAlgorithmInterfacePtr;

class BifrostSendAlgorithm {
 public:
  BifrostSendAlgorithm();
  ~BifrostSendAlgorithm();

 public:

 private:
  BifrostSendAlgorithmInterfacePtr algorithm_interface_;
};

}

#endif  //_BIFROST_SEND_ALGORITHM_H

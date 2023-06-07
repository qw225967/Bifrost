/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 4:17 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : setting.h
 * @description : TODO
 *******************************************************/

#ifndef WORKER_SETTING_H
#define WORKER_SETTING_H

#include <map>
#include <string>
#include <vector>

#include "common.h"

namespace bifrost {
class Settings {
 public:
  // Struct holding the configuration.
  struct Configuration {
    uint16_t rtcPort{9000u};
    std::string rtcIp{"::"};
    std::string dtlsCertificateFile;
    std::string dtlsPrivateKeyFile;
  };

 public:
  static void SetConfiguration(int argc, char* argv[]);
  static void PrintConfiguration();

 public:
  static struct Configuration configuration;
};
}  // namespace bifrost

#endif  // WORKER_SETTING_H

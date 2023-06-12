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
    Configuration(std::string name = "default", std::string ip = "127.0.0.1",
                  uint16_t port = 9000)
        : userName(std::move(name)), rtcIp(std::move(ip)), rtcPort(port) {}
    std::string userName;
    uint16_t rtcPort;
    std::string rtcIp;
  };

 public:
  static void SetConfiguration(int argc, char* argv[]);
  static void AnalysisConfigurationFile(std::string config_path);
  static void PrintConfiguration();

 public:
  static sockaddr get_sockaddr_by_config(Configuration config);

 public:
  static struct Configuration server_configuration_;
  static std::map<std::string, Configuration> client_configuration_map_;
};
}  // namespace bifrost

#endif  // WORKER_SETTING_H

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

#include <json.hpp>
#include <map>
#include <string>
#include <vector>

#include "common.h"
#include "experiment_manager.h"

namespace bifrost {
class Settings {
  using json = nlohmann::json;

 public:
  // Struct holding the configuration.
  struct Configuration {
    Configuration(std::string name = "default", std::string ip = "127.0.0.1",
                  uint16_t port = 9001, uint32_t ssrc = 12341234)
        : userName(std::move(name)),
          rtcIp(std::move(ip)),
          rtcPort(port),
          ssrc(ssrc) {}
    std::string userName;
    uint16_t rtcPort;
    std::string rtcIp;
    uint32_t ssrc;
  };
  struct AddressConfiguration {
    Configuration local_receive_configuration_;
    Configuration remote_send_configuration_;
  };

 public:
  static void SetConfiguration(int argc, char* argv[]);
  static void AnalysisConfigurationFile(std::string& config_path,
                                        std::string& player_config_path);
  static void ReadExperimentConfiguration(json& config);
  static void PrintConfiguration();

 public:
  static sockaddr get_sockaddr_by_config(Configuration& config);

 public:
  static struct AddressConfiguration publisher_config_;
  static std::map<std::string, struct AddressConfiguration> player_config_map_;
  static ExperimentManager::GccExperimentConfig gcc_experiment_config_;
};
}  // namespace bifrost

#endif  // WORKER_SETTING_H

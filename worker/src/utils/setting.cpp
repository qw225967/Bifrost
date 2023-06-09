/*******************************************************
 * @author      : dog head
 * @date        : Created in 2023/6/7 4:19 下午
 * @mail        : qw225967@github.com
 * @project     : worker
 * @file        : setting.cpp
 * @description : TODO
 *******************************************************/

#include "setting.h"

#include <cctype>  // isprint()
#include <fstream>
#include <iostream>
#include <iterator>  // std::ostream_iterator
#include <json.hpp>
extern "C" {
#include <getopt.h>
}

namespace bifrost {

/* Class variables. */
struct Settings::Configuration Settings::server_configuration_;
std::map<std::string, Settings::Configuration>
    Settings::client_configuration_map_;

/* Class methods. */
void Settings::SetConfiguration(int argc, char* argv[]) {
  /* Variables for getopt. */

  int c;
  int optionIdx{0};
  // clang-format off
  struct option options[] =
      {
      { "rtcPort",             optional_argument, nullptr, 'P' },
      { "rtcIp",               optional_argument, nullptr, 'I' },
      { nullptr, 0, nullptr, 0 }
      };
  // clang-format on
  std::string stringValue;
  std::vector<std::string> logTags;

  /* Parse command line options. */

  opterr = 0;  // Don't allow getopt to print error messages.
  while ((c = getopt_long_only(argc, argv, "", options, &optionIdx)) != -1) {
    if (!optarg)
      std::cout << "[setting] unknown configuration parameter: " << optarg
                << std::endl;

    switch (c) {
      case 'P': {
        try {
          Settings::server_configuration_.rtcPort =
              static_cast<uint16_t>(std::stoi(optarg));
        } catch (const std::exception& error) {
          std::cout << "[setting] %s" << error.what() << std::endl;
        }

        break;
      }

      case 'I': {
        try {
          stringValue = std::string(optarg);
          Settings::server_configuration_.rtcIp = stringValue;
        } catch (const std::exception& error) {
          std::cout << "[setting] %s" << error.what() << std::endl;
        }

        break;
      }

      // This should never happen.
      default: {
        std::cout << "[setting] 'default' should never happen" << std::endl;
      }
    }
  }
}

void Settings::PrintConfiguration() {}

void Settings::AnalysisConfigurationFile(std::string config_path) {
  using json = nlohmann::json;

  if (config_path.empty()) {
    return;
  }

  std::ifstream json_file(config_path.c_str());
  json config;
  json_file >> config;

  auto server_iter = config.find("ServerConfig");
  if (server_iter == config.end()) {
    std::cout << "[setting] server config can not find";
  } else {
    std::string server_ip;
    std::string server_name;
    uint16_t server_port;
    // name
    auto name_iter = server_iter->find("userName");
    if (name_iter == server_iter->end()) {
      std::cout << "[setting] server config can not find name";
    } else {
      server_name = name_iter->get<std::string>();
    }

    // ip
    auto ip_iter = server_iter->find("rtcIp");
    if (ip_iter == server_iter->end()) {
      std::cout << "[setting] server config can not find ip";
    } else {
      server_ip = ip_iter->get<std::string>();
    }

    // port
    auto port_iter = server_iter->find("rtcPort");
    if (port_iter == server_iter->end()) {
      std::cout << "[setting] server config can not find port";
    } else {
      server_port = port_iter->get<uint16_t>();
    }

    Configuration server_config(server_name, server_ip, server_port);
    server_configuration_ = server_config;
  }

  for (auto client : config["ClientConfigs"]) {
    std::string client_ip;
    std::string client_name;
    uint16_t client_port;
    // name
    auto name_iter = client.find("userName");
    if (name_iter == client.end()) {
      std::cout << "[setting] client config can not find name";
    } else {
      client_name = name_iter->get<std::string>();
    }

    // ip
    auto ip_iter = client.find("rtcIp");
    if (ip_iter == client.end()) {
      std::cout << "[setting] client config can not find ip";
    } else {
      client_ip = ip_iter->get<std::string>();
    }

    // port
    auto port_iter = client.find("rtcPort");
    if (port_iter == client.end()) {
      std::cout << "[setting] client config can not find port";
    } else {
      client_port = port_iter->get<uint16_t>();
    }

    Configuration client_config(client_name, client_ip, client_port);
    client_configuration_map_[client_name] = client_config;
  }
}

}  // namespace bifrost

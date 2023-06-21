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

#include "utils.h"
extern "C" {
#include <getopt.h>
}

namespace bifrost {

/* Class variables. */
struct Settings::AddressConfiguration Settings::publisher_config_;
std::map<std::string, struct Settings::AddressConfiguration>
    Settings::player_config_map_;

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
          Settings::publisher_config_.local_receive_configuration_.rtcPort =
              static_cast<uint16_t>(std::stoi(optarg));
        } catch (const std::exception& error) {
          std::cout << "[setting] %s" << error.what() << std::endl;
        }

        break;
      }

      case 'I': {
        try {
          stringValue = std::string(optarg);
          Settings::publisher_config_.local_receive_configuration_.rtcIp =
              stringValue;
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

sockaddr Settings::get_sockaddr_by_config(Configuration& publish_config) {
  std::string ip(publish_config.rtcIp);
  uint16_t port(publish_config.rtcPort);

  sockaddr remote_addr;

  int err;

  switch (IP::get_family(ip)) {
    case AF_INET: {
      err = uv_ip4_addr(ip.c_str(), static_cast<int>(port),
                        reinterpret_cast<struct sockaddr_in*>(&remote_addr));

      if (err != 0)
        std::cout << "[setting] uv_ip4_addr() failed: " << uv_strerror(err)
                  << std::endl;

      break;
    }

    case AF_INET6: {
      err = uv_ip6_addr(ip.c_str(), static_cast<int>(port),
                        reinterpret_cast<struct sockaddr_in6*>(&remote_addr));

      if (err != 0)
        std::cout << "[setting] uv_ip6_addr() failed: " << uv_strerror(err)
                  << std::endl;

      break;
    }

    default: {
      std::cout << "[setting] invalid IP " << ip.c_str() << std::endl;
    }
  }

  return remote_addr;
}

void Settings::PrintConfiguration() {}

void Settings::AnalysisConfigurationFile(std::string& publish_config_path,
                                         std::string& player_config_path) {
  using json = nlohmann::json;

  if (publish_config_path.empty()) {
    return;
  }

  std::ifstream publish_json_file(publish_config_path.c_str());
  json publish_config;
  publish_json_file >> publish_config;

  auto publish_receive_iter = publish_config.find("LocalReceiveConfigs");
  if (publish_receive_iter == publish_config.end()) {
    std::cout << "[setting] server publish_config can not find";
  } else {
    std::string ip;
    std::string name;
    uint16_t port;
    uint32_t ssrc;
    // name
    auto publish_name_iter = publish_receive_iter->find("userName");
    if (publish_name_iter == publish_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find name";
    } else {
      name = publish_name_iter->get<std::string>();
    }

    // ip
    auto publish_ip_iter = publish_receive_iter->find("rtcIp");
    if (publish_ip_iter == publish_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find ip";
    } else {
      ip = publish_ip_iter->get<std::string>();
    }

    // port
    auto publish_port_iter = publish_receive_iter->find("rtcPort");
    if (publish_port_iter == publish_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find port";
    } else {
      port = publish_port_iter->get<uint16_t>();
    }

    // ssrc
    auto publish_ssrc_iter = publish_receive_iter->find("ssrc");
    if (publish_ssrc_iter == publish_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find ssrc";
    } else {
      ssrc = publish_ssrc_iter->get<uint32_t>();
    }

    Configuration publish_receive_config(name, ip, port, ssrc);
    publisher_config_.local_receive_configuration_ = publish_receive_config;
  }

  auto publish_send_iter = publish_config.find("RemoteSendConfigs");
  if (publish_send_iter == publish_config.end()) {
    std::cout << "[setting] server publish_config can not find";
  } else {
    std::string ip;
    std::string name;
    uint16_t port;
    uint32_t ssrc;
    // name
    auto publish_name_iter = publish_send_iter->find("userName");
    if (publish_name_iter == publish_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find name";
    } else {
      name = publish_name_iter->get<std::string>();
    }

    // ip
    auto publish_ip_iter = publish_send_iter->find("rtcIp");
    if (publish_ip_iter == publish_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find ip";
    } else {
      ip = publish_ip_iter->get<std::string>();
    }

    // port
    auto publish_port_iter = publish_send_iter->find("rtcPort");
    if (publish_port_iter == publish_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find port";
    } else {
      port = publish_port_iter->get<uint16_t>();
    }

    // ssrc
    auto publish_ssrc_iter = publish_send_iter->find("ssrc");
    if (publish_ssrc_iter == publish_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find ssrc";
    } else {
      ssrc = publish_ssrc_iter->get<uint32_t>();
    }

    Configuration send_config(name, ip, port, ssrc);
    publisher_config_.remote_send_configuration_ = send_config;
  }

  if (player_config_path.empty()) {
    return;
  }

  std::ifstream player_json_file(player_config_path.c_str());
  json player_config;
  player_json_file >> player_config;

  for (auto config : player_config["Players"]) {
    AddressConfiguration addr_config;
    auto player_receive_iter = config.find("LocalReceiveConfigs");
    if (player_receive_iter == config.end()) {
      std::cout << "[setting] server publish_config can not find";
    } else {
      std::string ip;
      std::string name;
      uint16_t port;
      uint32_t ssrc;
      // name
      auto player_name_iter = player_receive_iter->find("userName");
      if (player_name_iter == player_receive_iter->end()) {
        std::cout << "[setting] server publish_config can not find name";
      } else {
        name = player_name_iter->get<std::string>();
      }

      // ip
      auto player_ip_iter = player_receive_iter->find("rtcIp");
      if (player_ip_iter == player_receive_iter->end()) {
        std::cout << "[setting] server publish_config can not find ip";
      } else {
        ip = player_ip_iter->get<std::string>();
      }

      // port
      auto player_port_iter = player_receive_iter->find("rtcPort");
      if (player_port_iter == player_receive_iter->end()) {
        std::cout << "[setting] server publish_config can not find port";
      } else {
        port = player_port_iter->get<uint16_t>();
      }

      // ssrc
      auto player_ssrc_iter = player_receive_iter->find("ssrc");
      if (player_ssrc_iter == player_receive_iter->end()) {
        std::cout << "[setting] server publish_config can not find ssrc";
      } else {
        ssrc = player_ssrc_iter->get<uint32_t>();
      }

      Configuration publish_receive_config(name, ip, port, ssrc);
      addr_config.local_receive_configuration_ = publish_receive_config;
      player_config_map_[name] = addr_config;
    }

    auto player_send_iter = config.find("RemoteSendConfigs");
    if (player_send_iter == config.end()) {
      std::cout << "[setting] server publish_config can not find";
    } else {
      std::string ip;
      std::string name;
      uint16_t port;
      uint32_t ssrc;
      // name
      auto player_name_iter = player_send_iter->find("userName");
      if (player_name_iter == player_send_iter->end()) {
        std::cout << "[setting] server publish_config can not find name";
      } else {
        name = player_name_iter->get<std::string>();
      }

      // ip
      auto player_ip_iter = player_send_iter->find("rtcIp");
      if (player_ip_iter == player_send_iter->end()) {
        std::cout << "[setting] server publish_config can not find ip";
      } else {
        ip = player_ip_iter->get<std::string>();
      }

      // port
      auto player_port_iter = player_send_iter->find("rtcPort");
      if (player_port_iter == player_send_iter->end()) {
        std::cout << "[setting] server publish_config can not find port";
      } else {
        port = player_port_iter->get<uint16_t>();
      }

      // ssrc
      auto player_ssrc_iter = player_send_iter->find("ssrc");
      if (player_ssrc_iter == player_send_iter->end()) {
        std::cout << "[setting] server publish_config can not find ssrc";
      } else {
        ssrc = player_ssrc_iter->get<uint32_t>();
      }

      Configuration send_config(name, ip, port, ssrc);
      addr_config.remote_send_configuration_ = send_config;
      player_config_map_[addr_config.local_receive_configuration_.userName] =
          addr_config;
    }
  }
}

}  // namespace bifrost

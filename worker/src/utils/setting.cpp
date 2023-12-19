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

#include "utils.h"
extern "C" {
#include <getopt.h>
}

namespace bifrost {

/* Class variables. */
struct Settings::AddressConfiguration Settings::config_;
struct ExperimentManager::GccExperimentConfig Settings::gcc_experiment_config_;

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
          Settings::config_.local_receive_configuration_.rtcPort =
              static_cast<uint16_t>(std::stoi(optarg));
        } catch (const std::exception& error) {
          std::cout << "[setting] %s" << error.what() << std::endl;
        }

        break;
      }

      case 'I': {
        try {
          stringValue = std::string(optarg);
          Settings::config_.local_receive_configuration_.rtcIp =
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

void Settings::ReadExperimentConfiguration(json& config) {
  auto gcc_iter = config.find("GccExperiment");
  if (gcc_iter == config.end()) {
    std::cout << "[setting] read experiment configuration gcc experiment"
              << std::endl;
  } else {
    auto tlws = gcc_iter->find("TrendLineWindowSize");
    gcc_experiment_config_.TrendLineWindowSize = tlws->get<uint32_t>();
    auto tlts = gcc_iter->find("TrendLineThreshold");
    gcc_experiment_config_.TrendLineThreshold = tlts->get<float>();
  }
}

void Settings::AnalysisConfigurationFile(std::string& config_path) {
  if (config_path.empty()) {
    return;
  }

  std::ifstream json_file(config_path.c_str());
  json config;
  json_file >> config;

  auto local_receive_iter = config.find("LocalReceiveConfigs");
  if (local_receive_iter == config.end()) {
    std::cout << "[setting] server publish_config can not find";
  } else {
    std::string ip;
    std::string name;
    uint16_t port;
    uint32_t ssrc;
    // name
    auto local_name_iter = local_receive_iter->find("userName");
    if (local_name_iter == local_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find name";
    } else {
      name = local_name_iter->get<std::string>();
    }

    // ip
    auto local_ip_iter = local_receive_iter->find("rtcIp");
    if (local_ip_iter == local_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find ip";
    } else {
      ip = local_ip_iter->get<std::string>();
    }

    // port
    auto local_port_iter = local_receive_iter->find("rtcPort");
    if (local_port_iter == local_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find port";
    } else {
      port = local_port_iter->get<uint16_t>();
    }

    // ssrc
    auto local_ssrc_iter = local_receive_iter->find("ssrc");
    if (local_ssrc_iter == local_receive_iter->end()) {
      std::cout << "[setting] server publish_config can not find ssrc" << std::endl;
    } else {
      ssrc = local_ssrc_iter->get<uint32_t>();
    }

    Configuration publish_receive_config(name, ip, port, ssrc);
    config_.local_receive_configuration_ = publish_receive_config;
  }

  auto remote_send_iter = config.find("RemoteSendConfigs");
  if (remote_send_iter == config.end()) {
    std::cout << "[setting] server publish_config can not find";
  } else {
    std::string ip;
    std::string name;
    uint16_t port;
    uint32_t ssrc;
    // name
    auto remote_name_iter = remote_send_iter->find("userName");
    if (remote_name_iter == remote_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find name";
    } else {
      name = remote_name_iter->get<std::string>();
    }

    // ip
    auto remote_ip_iter = remote_send_iter->find("rtcIp");
    if (remote_ip_iter == remote_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find ip";
    } else {
      ip = remote_ip_iter->get<std::string>();
    }

    // port
    auto remote_port_iter = remote_send_iter->find("rtcPort");
    if (remote_port_iter == remote_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find port";
    } else {
      port = remote_port_iter->get<uint16_t>();
    }

    // ssrc
    auto remote_ssrc_iter = remote_send_iter->find("ssrc");
    if (remote_ssrc_iter == remote_send_iter->end()) {
      std::cout << "[setting] server publish_config can not find ssrc" << std::endl;
    } else {
      ssrc = remote_ssrc_iter->get<uint32_t>();
    }

    Configuration send_config(name, ip, port, ssrc);
    config_.remote_send_configuration_ = send_config;
  }
    // experiment
    auto experiment = config.find("ExperimentConfig");
    ReadExperimentConfiguration(experiment.value());
}
}  // namespace bifrost

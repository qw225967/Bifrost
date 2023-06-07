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
#include <cerrno>
#include <iostream>
#include <iterator>  // std::ostream_iterator
#include <json.hpp>
extern "C" {
#include <getopt.h>
}

namespace bifrost {

/* Class variables. */
struct Settings::Configuration Settings::configuration;

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
          Settings::configuration.rtcPort =
              static_cast<uint16_t>(std::stoi(optarg));
        } catch (const std::exception& error) {
          std::cout << "[setting] %s" << error.what() << std::endl;
        }

        break;
      }

      case 'I': {
        try {
          stringValue = std::string(optarg);
          Settings::configuration.rtcIp = stringValue;
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
}  // namespace bifrost

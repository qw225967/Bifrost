# Install script for directory: /Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/base/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/algorithm/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/container/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/debugging/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/hash/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/memory/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/meta/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/numeric/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/strings/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/synchronization/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/time/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/types/cmake_install.cmake")
  include("/Users/admin/go/src/github.com/Bifrost/worker/third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/utility/cmake_install.cmake")

endif()


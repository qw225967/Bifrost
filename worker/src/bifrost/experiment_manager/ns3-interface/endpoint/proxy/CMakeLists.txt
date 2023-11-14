cmake_minimum_required(VERSION 3.20)
project(proxy)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(
        io
        libuv/docs/code/plugin
        libuv/include
        libuv/include/uv
        libuv/src
        libuv/src/unix
)

link_directories(libuv/build)

add_executable(proxy
        io/udp_socket.cpp
        io/udp_socket.h
        io/uv_run.h
        main.cpp
        ns3_proxy.cpp
        ns3_proxy.h
        rtc_header.h)

target_link_libraries(proxy libuv.a pthread dl)
cmake_minimum_required(VERSION 3.20)
project(worker)

set(CMAKE_CXX_STANDARD 14)


set(WEBRTC_DIR third_party/libwebrtc/libwebrtc)
set(WEBRTC_MODULES ${WEBRTC_DIR}/modules)

ADD_DEFINITIONS(-DWEBRTC_POSIX)

include_directories(
        include
        include/bifrost
        include/io
        include/rtc
        include/utils
        third_party/json
        third_party/json/include
        third_party/json/include/nlohmann
        third_party/json/include/nlohmann/detail
        third_party/json/include/nlohmann/detail/conversions
        third_party/json/include/nlohmann/detail/input
        third_party/json/include/nlohmann/detail/iterators
        third_party/json/include/nlohmann/detail/meta
        third_party/json/include/nlohmann/detail/output
        third_party/json/include/nlohmann/thirdparty
        third_party/json/include/nlohmann/thirdparty/hedley
        third_party/json/single_include
        third_party/json/single_include/nlohmann
        third_party/libuv/docs/code/plugin
        third_party/libuv/include
        third_party/libuv/include/uv
        third_party/libuv/src
        third_party/libuv/src/unix
        third_party/libuv/build
        third_party/libwebrtc/deps/abseil-cpp/abseil-cpp
        third_party/libwebrtc/libwebrtc/
)

aux_source_directory(third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source RTP_RTCP_SOURCE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/include RTP_RTCP_INCLUDE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/rtc_base RTC_BASE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/rtc_base/numerics RTC_BASE_NUMERICS_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/system_wrappers/source SYSTEM_WRAPPERS_CC)
aux_source_directory(third_party/libwebrtc/deps/abseil-cpp/abseil-cpp ABSL_CC)
aux_source_directory(third_party/libwebrtc/deps/abseil-cpp/abseil-cpp/absl/types ABSL_TYPE_CC)

add_executable(worker

        ${RTP_RTCP_INCLUDE_CC}
        ${RTP_RTCP_SOURCE_CC}
        ${RTC_BASE_CC}
        ${RTC_BASE_NUMERICS_CC}
        ${SYSTEM_WRAPPERS_CC}
        ${ABSL_CC}

        include/rtc/tcc_client.hpp
        include/rtc/tcc_server.hpp
        src/rtc/tcc_client.cpp
        src/rtc/tcc_server.cpp
        include/io/uv_timer.h
        include/bifrost/data_producer.h
        include/io/port_manager.h
        include/io/udp_socket.h
        include/io/uv_loop.h
        include/rtc/payload_descriptor.h
        include/rtc/rtp_packet.h
        include/rtc/transport.h
        include/rtc/udp_router.h
        include/utils/common.h
        include/utils/setting.h
        include/utils/utils.h
        src/io/uv_timer.cpp
        src/bifrost/data_producer.cpp
        src/io/port_manager.cpp
        src/io/udp_socket.cpp
        src/io/uv_loop.cpp
        src/rtc/rtp_packet.cpp
        src/rtc/transport.cpp
        src/rtc/udp_router.cpp
        src/utils/setting.cpp
        src/utils/utils.cpp
        src/main.cpp
        third_party/json/include/nlohmann/detail/conversions/from_json.hpp
        third_party/json/include/nlohmann/detail/conversions/to_chars.hpp
        third_party/json/include/nlohmann/detail/conversions/to_json.hpp
        third_party/json/include/nlohmann/detail/input/binary_reader.hpp
        third_party/json/include/nlohmann/detail/input/input_adapters.hpp
        third_party/json/include/nlohmann/detail/input/json_sax.hpp
        third_party/json/include/nlohmann/detail/input/lexer.hpp
        third_party/json/include/nlohmann/detail/input/parser.hpp
        third_party/json/include/nlohmann/detail/input/position_t.hpp
        third_party/json/include/nlohmann/detail/iterators/internal_iterator.hpp
        third_party/json/include/nlohmann/detail/iterators/iter_impl.hpp
        third_party/json/include/nlohmann/detail/iterators/iteration_proxy.hpp
        third_party/json/include/nlohmann/detail/iterators/iterator_traits.hpp
        third_party/json/include/nlohmann/detail/iterators/json_reverse_iterator.hpp
        third_party/json/include/nlohmann/detail/iterators/primitive_iterator.hpp
        third_party/json/include/nlohmann/detail/meta/cpp_future.hpp
        third_party/json/include/nlohmann/detail/meta/detected.hpp
        third_party/json/include/nlohmann/detail/meta/is_sax.hpp
        third_party/json/include/nlohmann/detail/meta/type_traits.hpp
        third_party/json/include/nlohmann/detail/meta/void_t.hpp
        third_party/json/include/nlohmann/detail/output/binary_writer.hpp
        third_party/json/include/nlohmann/detail/output/output_adapters.hpp
        third_party/json/include/nlohmann/detail/output/serializer.hpp
        third_party/json/include/nlohmann/detail/exceptions.hpp
        third_party/json/include/nlohmann/detail/hash.hpp
        third_party/json/include/nlohmann/detail/json_pointer.hpp
        third_party/json/include/nlohmann/detail/json_ref.hpp
        third_party/json/include/nlohmann/detail/macro_scope.hpp
        third_party/json/include/nlohmann/detail/macro_unscope.hpp
        third_party/json/include/nlohmann/detail/value_t.hpp
        third_party/json/include/nlohmann/thirdparty/hedley/hedley.hpp
        third_party/json/include/nlohmann/thirdparty/hedley/hedley_undef.hpp
        third_party/json/include/nlohmann/adl_serializer.hpp
        third_party/json/include/nlohmann/byte_container_with_subtype.hpp
        third_party/json/include/nlohmann/json.hpp
        third_party/json/include/nlohmann/json_fwd.hpp
        third_party/json/include/nlohmann/ordered_map.hpp
        third_party/json/single_include/nlohmann/json.hpp)

target_link_libraries(worker libuv.a)

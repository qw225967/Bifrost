cmake_minimum_required(VERSION 3.20)
project(worker)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_BUILD_TYPE Debug)


include_directories(include)
include_directories(include/io)
include_directories(include/rtc)
include_directories(include/utils)
include_directories(third_party/json)
include_directories(third_party/json/include)
include_directories(third_party/json/include/nlohmann)
include_directories(third_party/json/include/nlohmann/detail)
include_directories(third_party/json/include/nlohmann/detail/conversions)
include_directories(third_party/json/include/nlohmann/detail/input)
include_directories(third_party/json/include/nlohmann/detail/iterators)
include_directories(third_party/json/include/nlohmann/detail/meta)
include_directories(third_party/json/include/nlohmann/detail/output)
include_directories(third_party/json/include/nlohmann/thirdparty)
include_directories(third_party/json/include/nlohmann/thirdparty/hedley)
include_directories(third_party/json/single_include)
include_directories(third_party/json/single_include/nlohmann)
include_directories(third_party/libuv/include)
include_directories(third_party/libuv/include/uv)

link_directories(third_party/libuv/build)

add_executable(worker
        include/io/port_manager.h
        include/io/udp_socket.h
        include/io/uv_loop.h
        include/rtc/udp_router.h
        include/utils/common.h
        include/utils/setting.h
        include/utils/utils.h
        src/io/port_manager.cpp
        src/io/udp_socket.cpp
        src/io/uv_loop.cpp
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

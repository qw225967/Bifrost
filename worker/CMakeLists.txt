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
        third_party/libwebrtc/deps/abseil-cpp/abseil-cpp
        third_party/libwebrtc/libwebrtc/
)


aux_source_directory(third_party/libwebrtc/libwebrtc/call CALL_SOURCE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/include RTP_RTCP_INCLUDE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source RTP_RTCP_SOURCE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/rtc_base RTC_BASE_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/rtc_base/numerics RTC_BASE_NUMERICS_CC)
aux_source_directory(third_party/libwebrtc/libwebrtc/system_wrappers/source SYSTEM_WRAPPERS_CC)

add_subdirectory(third_party/libwebrtc/deps/abseil-cpp/abseil-cpp)

link_directories(third_party/libuv/build)

add_executable(worker
        # C++ source files.
        third_party/libwebrtc/libwebrtc/system_wrappers/source/field_trial.cc
        third_party/libwebrtc/libwebrtc/rtc_base/rate_statistics.cc
        third_party/libwebrtc/libwebrtc/rtc_base/experiments/field_trial_parser.cc
        third_party/libwebrtc/libwebrtc/rtc_base/experiments/alr_experiment.cc
        third_party/libwebrtc/libwebrtc/rtc_base/experiments/field_trial_units.cc
        third_party/libwebrtc/libwebrtc/rtc_base/experiments/rate_control_settings.cc
        third_party/libwebrtc/libwebrtc/rtc_base/network/sent_packet.cc
        third_party/libwebrtc/libwebrtc/call/rtp_transport_controller_send.cc
        third_party/libwebrtc/libwebrtc/api/transport/bitrate_settings.cc
        third_party/libwebrtc/libwebrtc/api/transport/field_trial_based_config.cc
        third_party/libwebrtc/libwebrtc/api/transport/network_types.cc
        third_party/libwebrtc/libwebrtc/api/transport/goog_cc_factory.cc
        third_party/libwebrtc/libwebrtc/api/units/timestamp.cc
        third_party/libwebrtc/libwebrtc/api/units/time_delta.cc
        third_party/libwebrtc/libwebrtc/api/units/data_rate.cc
        third_party/libwebrtc/libwebrtc/api/units/data_size.cc
        third_party/libwebrtc/libwebrtc/api/units/frequency.cc
        third_party/libwebrtc/libwebrtc/api/network_state_predictor.cc
        third_party/libwebrtc/libwebrtc/modules/pacing/interval_budget.cc
        third_party/libwebrtc/libwebrtc/modules/pacing/bitrate_prober.cc
        third_party/libwebrtc/libwebrtc/modules/pacing/paced_sender.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/overuse_detector.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/overuse_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/aimd_rate_control.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/inter_arrival.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/bwe_defines.cc
        third_party/libwebrtc/libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.cc
        third_party/libwebrtc/libwebrtc/modules/bitrate_controller/send_side_bandwidth_estimation.cc
        third_party/libwebrtc/libwebrtc/modules/bitrate_controller/loss_based_bandwidth_estimation.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/goog_cc_network_control.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/probe_bitrate_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/congestion_window_pushback_controller.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/link_capacity_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/alr_detector.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/probe_controller.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/median_slope_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/bitrate_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/trendline_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/delay_based_bwe.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/rtp/send_time_history.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/rtp/transport_feedback_adapter.cc
        third_party/libwebrtc/libwebrtc/modules/congestion_controller/rtp/control_handler.cc
        # C++ source files fec.
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/flexfec_header_reader_writer.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/ulpfec_header_reader_writer.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/flexfec_receiver.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/forward_error_correction.cc
        third_party/libwebrtc/libwebrtc/modules/audio_coding/codecs/cng/webrtc_cng.cc
        third_party/libwebrtc/libwebrtc/modules/audio_coding/neteq/packet.cc
        third_party/libwebrtc/libwebrtc/modules/audio_coding/neteq/red_payload_splitter.cc
        third_party/libwebrtc/libwebrtc/modules/audio_coding/neteq/decoder_database.cc
        third_party/libwebrtc/libwebrtc/api/rtp_headers.cc
        third_party/libwebrtc/libwebrtc/api/media_types.cc
        third_party/libwebrtc/libwebrtc/api/audio_options.cc
        third_party/libwebrtc/libwebrtc/api/media_stream_interface.cc
        third_party/libwebrtc/libwebrtc/api/rtp_packet_info.cc
        third_party/libwebrtc/libwebrtc/api/rtp_parameters.cc
        third_party/libwebrtc/libwebrtc/api/neteq/tick_timer.cc
        third_party/libwebrtc/libwebrtc/api/video/hdr_metadata.cc
        third_party/libwebrtc/libwebrtc/api/video/video_content_type.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_format.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_decoder.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_codec_pair_id.cc
        third_party/libwebrtc/libwebrtc/rtc_base/copy_on_write_buffer.cc
        third_party/libwebrtc/libwebrtc/rtc_base/critical_section.cc
        third_party/libwebrtc/libwebrtc/rtc_base/logging.cc
        third_party/libwebrtc/libwebrtc/rtc_base/zero_memory.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/clock.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/sequence_checker.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/rw_lock_wrapper.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/rw_lock_posix.cc
        third_party/libwebrtc/libwebrtc/rtc_base/checks.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_packet.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/forward_error_correction_internal.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_header_extension_map.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_header_extensions.cc
        third_party/libwebrtc/libwebrtc/rtc_base/time_utils.cc
        third_party/libwebrtc/libwebrtc/rtc_base/string_utils.cc
        third_party/libwebrtc/libwebrtc/rtc_base/string_encode.cc
        third_party/libwebrtc/libwebrtc/rtc_base/strings/string_builder.cc
        third_party/libwebrtc/libwebrtc/rtc_base/platform_thread_types.cc
        third_party/libwebrtc/libwebrtc/rtc_base/strings/audio_format_to_string.cc
        third_party/libwebrtc/libwebrtc/api/task_queue/task_queue_base.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/fec_private_tables_random.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/fec_private_tables_bursty.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_dependency_descriptor_extension.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_packet_received.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/spl_sqrt.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/randomization_functions.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/vector_scaling_operations.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/filter_ar.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/energy.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/division_operations.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/get_hanning_window.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/ilbc_specific_functions.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/auto_correlation.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/levinson_durbin.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/copy_set_operations.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/get_scaling_square.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/min_max_operations.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/spl_init.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/cross_correlation.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/downsample_fast.cc

        #        ${RTP_RTCP_INCLUDE_CC}
        #        ${RTP_RTCP_SOURCE_CC}
        #        ${RTC_BASE_CC}
        #        ${RTC_BASE_NUMERICS_CC}
        #        ${SYSTEM_WRAPPERS_CC}
        #        ${CALL_SOURCE_CC}
        #        ${API_VIDEO_SOURCE_CC}

        # include
        include/bifrost/data_producer.h
        include/io/uv_timer.h
        include/io/port_manager.h
        include/io/udp_socket.h
        include/io/uv_loop.h
        include/rtc/payload_descriptor.h
        include/rtc/rtp_packet.h
        include/rtc/transport.h
        include/rtc/udp_router.h
        include/rtc/rtcp_packet.h
        include/rtc/rtcp_feedback.h
        include/rtc/rtcp_tcc.h
        include/rtc/tcc_client.h
        include/rtc/tcc_server.h
        include/rtc/sequence_manager.h
        include/utils/common.h
        include/utils/setting.h
        include/utils/utils.h

        # src
        src/bifrost/data_producer.cpp
        src/io/uv_timer.cpp
        src/io/port_manager.cpp
        src/io/udp_socket.cpp
        src/io/uv_loop.cpp
        src/rtc/tcc_client.cpp
        src/rtc/tcc_server.cpp
        src/rtc/rtcp_packet.cpp
        src/rtc/rtcp_feedback.cpp
        src/rtc/rtcp_tcc.cpp
        src/rtc/rtp_packet.cpp
        src/rtc/transport.cpp
        src/rtc/udp_router.cpp
        src/rtc/sequence_manager.cpp
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
        third_party/json/single_include/nlohmann/json.hpp
        )

target_link_libraries(worker libuv.a)

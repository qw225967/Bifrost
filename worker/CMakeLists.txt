cmake_minimum_required(VERSION 3.2)
project(Bifrost)

set(CMAKE_CXX_STANDARD 17)

set(WEBRTC_DIR third_party/libwebrtc/libwebrtc)
set(WEBRTC_MODULES ${WEBRTC_DIR}/modules)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  # /wd4005  macro-redefinition
  # /wd4068  unknown pragma
  # /wd4244  conversion from 'type1' to 'type2'
  # /wd4267  conversion from 'size_t' to 'type2'
  # /wd4800  force value to bool 'true' or 'false' (performance warning)
  add_compile_options(/wd4005 /wd4068 /wd4244 /wd4267 /wd4800)
  add_definitions(/DNOMINMAX /DWIN32_LEAN_AND_MEAN=1 /D_CRT_SECURE_NO_WARNINGS)
endif()

# 定义WEBRTC平台特定的源文件
set(WEBRTC_PLATFORM_SOURCES "")

# 检查操作系统并添加相应的源文件
if (WIN32)
    list(APPEND WEBRTC_PLATFORM_SOURCES "third_party/libwebrtc/libwebrtc/rtc_base/synchronization/rw_lock_win.cc")
    add_definitions(-DWEBRTC_WIN)
else ()
    list(APPEND WEBRTC_PLATFORM_SOURCES "third_party/libwebrtc/libwebrtc/rtc_base/synchronization/rw_lock_posix.cc")
    add_definitions(-DWEBRTC_POSIX)
endif ()

include_directories(
        include
        include/bifrost
        include/io
        include/rtc
        include/utils
        third_party/libwebrtc/deps/abseil-cpp/abseil-cpp
        third_party/libwebrtc/libwebrtc
        third_party/libquic
        third_party/libns3/include # lib ns-3-dev : ns-3.29
        third_party/libns3
)

set(BUILD_TESTING OFF CACHE INTERNAL "")
add_subdirectory(third_party/libwebrtc/deps/abseil-cpp/abseil-cpp)
add_subdirectory(third_party/libuv)
add_subdirectory(third_party/json)

add_executable(worker
        # webrtc
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
        third_party/libwebrtc/libwebrtc/api/video/color_space.cc
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
        ${WEBRTC_PLATFORM_SOURCES}
        # C++ source files fec.
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/flexfec_header_reader_writer.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/ulpfec_header_reader_writer.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/flexfec_receiver.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_header_extension_size.cc
        third_party/libwebrtc/libwebrtc/rtc_base/random.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/forward_error_correction.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_packet_to_send.cc
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
        third_party/libwebrtc/libwebrtc/modules/video_coding/packet.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/receiver.cc
        third_party/libwebrtc/libwebrtc/api/video_codecs/video_decoder.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/metrics.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_format.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_decoder.cc
        third_party/libwebrtc/libwebrtc/api/audio_codecs/audio_codec_pair_id.cc
        third_party/libwebrtc/libwebrtc/rtc_base/copy_on_write_buffer.cc
        third_party/libwebrtc/libwebrtc/rtc_base/critical_section.cc
        third_party/libwebrtc/libwebrtc/rtc_base/logging.cc
        third_party/libwebrtc/libwebrtc/rtc_base/zero_memory.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/clock.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/event.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/sequence_checker.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/rw_lock_wrapper.cc
        third_party/libwebrtc/libwebrtc/rtc_base/checks.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_packet.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/forward_error_correction_internal.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_header_extension_map.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_header_extensions.cc
        third_party/libwebrtc/libwebrtc/modules/include/module_common_types.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_h264.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_video_generic.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_video_header.cc
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
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_h264.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_video_generic.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_vp8.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format_vp9.cc
        third_party/libwebrtc/libwebrtc/rtc_base/event_tracer.cc
        third_party/libwebrtc/libwebrtc/rtc_base/bit_buffer.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/packet_buffer.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/rtp_frame_reference_finder.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/jitter_buffer.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/frame_buffer.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/frame_object.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/session_info.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/decoding_state.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/jitter_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/rtt_filter.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/encoded_frame.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/timing.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/codec_timer.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/inter_frame_delay.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_format.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/remote_ntp_time_estimator.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/flexfec_sender.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/ulpfec_generator.cc
        third_party/libwebrtc/libwebrtc/modules/rtp_rtcp/source/rtp_utility.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/media_opt_util.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/utility/simulcast_rate_allocator.cc
        third_party/libwebrtc/libwebrtc/api/video/video_bitrate_allocation.cc
        third_party/libwebrtc/libwebrtc/rtc_base/numerics/exp_filter.cc
        third_party/libwebrtc/libwebrtc/api/video/video_timing.cc
        third_party/libwebrtc/libwebrtc/rtc_base/event.cc
        third_party/libwebrtc/libwebrtc/rtc_base/synchronization/yield_policy.cc
        third_party/libwebrtc/libwebrtc/common_video/generic_frame_descriptor/generic_frame_info.cc
        third_party/libwebrtc/libwebrtc/rtc_base/time/timestamp_extrapolator.cc
        third_party/libwebrtc/libwebrtc/api/video/encoded_image.cc
        third_party/libwebrtc/libwebrtc/api/video/encoded_frame.cc
        third_party/libwebrtc/libwebrtc/modules/video_coding/include/video_codec_interface.cc
        third_party/libwebrtc/libwebrtc/rtc_base/experiments/jitter_upper_bound_experiment.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/rtp_to_ntp_estimator.cc
        third_party/libwebrtc/libwebrtc/common_audio/signal_processing/spl_inl.c
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
        third_party/libwebrtc/libwebrtc/common_video/h264/sps_vui_rewriter.cc
        third_party/libwebrtc/libwebrtc/common_video/h264/sps_parser.cc
        third_party/libwebrtc/libwebrtc/common_video/h264/pps_parser.cc
        third_party/libwebrtc/libwebrtc/common_video/h264/h264_common.cc
        third_party/libwebrtc/libwebrtc/rtc_base/bit_buffer.cc
        third_party/libwebrtc/libwebrtc/system_wrappers/source/metrics.cc

        # libquic
        third_party/libquic/quiche/common/platform/default/quiche_platform_impl/quiche_flags_impl.h
        third_party/libquic/quiche/common/platform/default/quiche_platform_impl/quiche_export_impl.h
        third_party/libquic/quiche/common/platform/default/quiche_platform_impl/quiche_flag_utils_impl.h
        third_party/libquic/quiche/common/platform/default/quiche_platform_impl/quiche_flags_impl.cc
        third_party/libquic/quiche/common/platform/default/quiche_platform_impl/quiche_reference_counted_impl.h
        third_party/libquic/quiche/common/quiche_circular_deque.h
        third_party/libquic/quiche/common/quiche_random.h
        third_party/libquic/quiche/common/quiche_random.cc
        third_party/libquic/quiche/common/quiche_protocol_flags_list.h
        third_party/libquic/quiche/quic/core/congestion_control/bandwidth_sampler.cc
        third_party/libquic/quiche/quic/core/congestion_control/bandwidth_sampler.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_drain.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_drain.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_misc.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_misc.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_probe_bw.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_probe_bw.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_probe_rtt.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_probe_rtt.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_sender.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_sender.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_startup.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr2_startup.h
        third_party/libquic/quiche/quic/core/congestion_control/bbr_sender.cc
        third_party/libquic/quiche/quic/core/congestion_control/bbr_sender.h
        third_party/libquic/quiche/quic/core/congestion_control/cubic_bytes.cc
        third_party/libquic/quiche/quic/core/congestion_control/cubic_bytes.h
        third_party/libquic/quiche/quic/core/congestion_control/general_loss_algorithm.cc
        third_party/libquic/quiche/quic/core/congestion_control/general_loss_algorithm.h
        third_party/libquic/quiche/quic/core/congestion_control/loss_detection_interface.h
        third_party/libquic/quiche/quic/core/congestion_control/pacing_sender.cc
        third_party/libquic/quiche/quic/core/congestion_control/pacing_sender.h
        third_party/libquic/quiche/quic/core/congestion_control/prr_sender.cc
        third_party/libquic/quiche/quic/core/congestion_control/prr_sender.h
        third_party/libquic/quiche/quic/core/congestion_control/rtt_stats.cc
        third_party/libquic/quiche/quic/core/congestion_control/rtt_stats.h
        third_party/libquic/quiche/quic/core/congestion_control/send_algorithm_interface.cc
        third_party/libquic/quiche/quic/core/congestion_control/send_algorithm_interface.h
        third_party/libquic/quiche/quic/core/congestion_control/tcp_cubic_sender_bytes.cc
        third_party/libquic/quiche/quic/core/congestion_control/tcp_cubic_sender_bytes.h
        third_party/libquic/quiche/quic/core/congestion_control/uber_loss_algorithm.cc
        third_party/libquic/quiche/quic/core/congestion_control/uber_loss_algorithm.h
        third_party/libquic/quiche/quic/core/congestion_control/windowed_filter.h
        third_party/libquic/quiche/quic/core/crypto/crypto_protocol.h
        third_party/libquic/quiche/quic/core/crypto/quic_random.h
        third_party/libquic/quiche/quic/core/packet_number_indexed_queue.h
        third_party/libquic/quiche/quic/core/quic_ack_listener_interface.cc
        third_party/libquic/quiche/quic/core/quic_ack_listener_interface.h
        third_party/libquic/quiche/quic/core/quic_bandwidth.cc
        third_party/libquic/quiche/quic/core/quic_bandwidth.h
        third_party/libquic/quiche/quic/core/quic_clock.h
        third_party/libquic/quiche/quic/core/quic_connection_id.cc
        third_party/libquic/quiche/quic/core/quic_connection_id.h
        third_party/libquic/quiche/quic/core/quic_connection_stats.cc
        third_party/libquic/quiche/quic/core/quic_connection_stats.h
        third_party/libquic/quiche/quic/core/quic_constants.cc
        third_party/libquic/quiche/quic/core/quic_constants.h
        third_party/libquic/quiche/quic/core/quic_error_codes.cc
        third_party/libquic/quiche/quic/core/quic_error_codes.h
        third_party/libquic/quiche/quic/core/quic_flags_list.h
        third_party/libquic/quiche/quic/core/quic_packet_number.cc
        third_party/libquic/quiche/quic/core/quic_packet_number.h
        third_party/libquic/quiche/quic/core/quic_protocol_flags_list.h
        third_party/libquic/quiche/quic/core/quic_tag.cc
        third_party/libquic/quiche/quic/core/quic_tag.h
        third_party/libquic/quiche/quic/core/quic_time.cc
        third_party/libquic/quiche/quic/core/quic_time.h
        third_party/libquic/quiche/quic/core/quic_time_accumulator.h
        third_party/libquic/quiche/quic/core/quic_transmission_info.cc
        third_party/libquic/quiche/quic/core/quic_transmission_info.h
        third_party/libquic/quiche/quic/core/quic_types.cc
        third_party/libquic/quiche/quic/core/quic_types.h
        third_party/libquic/quiche/quic/core/quic_unacked_packet_map.cc
        third_party/libquic/quiche/quic/core/quic_unacked_packet_map.h
        third_party/libquic/quiche/quic/core/quic_utils.cc
        third_party/libquic/quiche/quic/core/quic_utils.h
        third_party/libquic/quiche/quic/core/quic_versions.cc
        third_party/libquic/quiche/quic/core/quic_versions.h
        third_party/libquic/quiche/quic/platform/api/quic_export.h
        third_party/libquic/quiche/quic/platform/api/quic_flag_utils.h
        third_party/libquic/quiche/quic/platform/api/quic_flags.h

        # bifrost include
        include/bifrost/experiment_manager/fake_data_producer.h
        include/bifrost/experiment_manager/experiment_manager.h
        include/bifrost/experiment_manager/experiment_data.h
        include/bifrost/experiment_manager/h264_file_data_producer.h
        include/bifrost/bifrost_send_algorithm/tcc_client.h
        include/bifrost/bifrost_send_algorithm/tcc_server.h
        include/bifrost/bifrost_send_algorithm/bifrost_pacer.h
        include/bifrost/bifrost_send_algorithm/bifrost_send_algorithm_interface.h
        include/bifrost/bifrost_send_algorithm/bifrost_send_algorithm_manager.h
        include/bifrost/bifrost_send_algorithm/quic_send_algorithm_adapter.h
        include/bifrost/bifrost_send_algorithm/quic_clock_adapter.h
        include/bifrost/player.h
        include/bifrost/publisher.h
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
        include/rtc/sequence_manager.h
        include/rtc/nack.h
        include/rtc/rtcp_nack.h
        include/rtc/rtcp_feedback_rtp.h
        include/rtc/rtcp_feedback_item.h
        include/rtc/rtcp_compound_packet.h
        include/rtc/rtcp_rr.h
        include/rtc/rtcp_sr.h
        include/rtc/rtcp_quic_feedback.h
        include/utils/common.h
        include/utils/setting.h
        include/utils/utils.h

        # bifrost src
        src/bifrost/experiment_manager/fake_data_producer.cpp
        src/bifrost/experiment_manager/experiment_manager.cpp
        src/bifrost/experiment_manager/h264_file_data_producer.cpp
        src/bifrost/bifrost_send_algorithm/bifrost_pacer.cpp
        src/bifrost/bifrost_send_algorithm/bifrost_send_algorithm_manager.cpp
        src/bifrost/bifrost_send_algorithm/quic_send_algorithm_adapter.cpp
        src/bifrost/bifrost_send_algorithm/tcc_client.cpp
        src/bifrost/bifrost_send_algorithm/tcc_server.cpp
        src/bifrost/player.cpp
        src/bifrost/publisher.cpp
        src/io/uv_timer.cpp
        src/io/port_manager.cpp
        src/io/udp_socket.cpp
        src/io/uv_loop.cpp
        src/rtc/rtcp_packet.cpp
        src/rtc/rtcp_feedback.cpp
        src/rtc/rtcp_tcc.cpp
        src/rtc/rtp_packet.cpp
        src/rtc/transport.cpp
        src/rtc/udp_router.cpp
        src/rtc/sequence_manager.cpp
        src/rtc/nack.cpp
        src/rtc/rtcp_nack.cpp
        src/rtc/rtcp_feedback_rtp.cpp
        src/rtc/rtcp_compound_packet.cpp
        src/rtc/rtcp_rr.cpp
        src/rtc/rtcp_sr.cpp
        src/rtc/rtcp_quic_feedback.cpp
        src/utils/setting.cpp
        src/utils/utils.cpp
        src/main.cpp
        )

# 根据平台设置要链接的库
set(PLATFORM_LIBRARIES nlohmann_json absl::base absl::utility absl::str_format absl::flat_hash_map absl::hashtablez_sampler)
if (WIN32)
    list(APPEND PLATFORM_LIBRARIES winmm)
else ()
    list(APPEND PLATFORM_LIBRARIES dl)
endif ()

target_link_libraries(worker uv_a ${PLATFORM_LIBRARIES})

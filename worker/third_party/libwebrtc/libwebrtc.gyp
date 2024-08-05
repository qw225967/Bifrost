{
  'target_defaults': {
    'dependencies':
    [
      'deps/abseil-cpp/abseil-cpp.gyp:abseil',
      '../libuv/uv.gyp:libuv',
      '../openssl/openssl.gyp:openssl'
    ],
    'defines':
    [
      'WEBRTC_POSIX'
    ],
    'direct_dependent_settings': {
      'include_dirs':
      [
        '.',
        'libwebrtc'
      ]
    },
    'sources':
    [
      # C++ source files.
      'libwebrtc/system_wrappers/source/field_trial.cc',
      'libwebrtc/rtc_base/rate_statistics.cc',
      'libwebrtc/rtc_base/experiments/field_trial_parser.cc',
      'libwebrtc/rtc_base/experiments/alr_experiment.cc',
      'libwebrtc/rtc_base/experiments/field_trial_units.cc',
      'libwebrtc/rtc_base/experiments/rate_control_settings.cc',
      'libwebrtc/rtc_base/network/sent_packet.cc',
      'libwebrtc/call/rtp_transport_controller_send.cc',
      'libwebrtc/api/transport/bitrate_settings.cc',
      'libwebrtc/api/transport/field_trial_based_config.cc',
      'libwebrtc/api/transport/network_types.cc',
      'libwebrtc/api/transport/goog_cc_factory.cc',
      'libwebrtc/api/units/timestamp.cc',
      'libwebrtc/api/units/time_delta.cc',
      'libwebrtc/api/units/data_rate.cc',
      'libwebrtc/api/units/data_size.cc',
      'libwebrtc/api/units/frequency.cc',
      'libwebrtc/api/network_state_predictor.cc',
      'libwebrtc/modules/pacing/interval_budget.cc',
      'libwebrtc/modules/pacing/bitrate_prober.cc',
      'libwebrtc/modules/pacing/paced_sender.cc',
      'libwebrtc/modules/remote_bitrate_estimator/overuse_detector.cc',
      'libwebrtc/modules/remote_bitrate_estimator/overuse_estimator.cc',
      'libwebrtc/modules/remote_bitrate_estimator/aimd_rate_control.cc',
      'libwebrtc/modules/remote_bitrate_estimator/inter_arrival.cc',
      'libwebrtc/modules/remote_bitrate_estimator/bwe_defines.cc',
      'libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.cc',
      'libwebrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.cc',
      'libwebrtc/modules/bitrate_controller/send_side_bandwidth_estimation.cc',
      'libwebrtc/modules/bitrate_controller/loss_based_bandwidth_estimation.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/goog_cc_network_control.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/probe_bitrate_estimator.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/congestion_window_pushback_controller.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/link_capacity_estimator.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/alr_detector.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/probe_controller.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/median_slope_estimator.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/bitrate_estimator.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/trendline_estimator.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/delay_based_bwe.cc',
      'libwebrtc/modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.cc',
      'libwebrtc/modules/congestion_controller/rtp/send_time_history.cc',
      'libwebrtc/modules/congestion_controller/rtp/transport_feedback_adapter.cc',
      'libwebrtc/modules/congestion_controller/rtp/control_handler.cc',
      # C++ source files fec.
      'libwebrtc/modules/rtp_rtcp/source/flexfec_header_reader_writer.cc',
      'libwebrtc/modules/rtp_rtcp/source/ulpfec_header_reader_writer.cc',
      'libwebrtc/modules/rtp_rtcp/source/flexfec_receiver.cc',
      'libwebrtc/modules/rtp_rtcp/source/forward_error_correction.cc',
      'libwebrtc/modules/audio_coding/codecs/cng/webrtc_cng.cc',
      'libwebrtc/modules/audio_coding/neteq/packet.cc',
      'libwebrtc/modules/audio_coding/neteq/red_payload_splitter.cc',
      'libwebrtc/modules/audio_coding/neteq/decoder_database.cc',
      'libwebrtc/api/rtp_headers.cc',
      'libwebrtc/api/media_types.cc',
      'libwebrtc/api/audio_options.cc',
      'libwebrtc/api/media_stream_interface.cc',
      'libwebrtc/api/rtp_packet_info.cc',
      'libwebrtc/api/rtp_parameters.cc',
      'libwebrtc/api/neteq/tick_timer.cc',
      'libwebrtc/api/video/hdr_metadata.cc',
      'libwebrtc/api/video/video_content_type.cc',
      'libwebrtc/api/audio_codecs/audio_format.cc',
      'libwebrtc/api/audio_codecs/audio_decoder.cc',
      'libwebrtc/api/audio_codecs/audio_codec_pair_id.cc',
      'libwebrtc/rtc_base/copy_on_write_buffer.cc',
      'libwebrtc/rtc_base/critical_section.cc',
      'libwebrtc/rtc_base/logging.cc',
      'libwebrtc/rtc_base/zero_memory.cc',
      'libwebrtc/system_wrappers/source/clock.cc',
      'libwebrtc/rtc_base/synchronization/sequence_checker.cc',
      'libwebrtc/rtc_base/synchronization/rw_lock_wrapper.cc',
      'libwebrtc/rtc_base/synchronization/rw_lock_posix.cc',
      'libwebrtc/rtc_base/checks.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_packet.cc',
      'libwebrtc/modules/rtp_rtcp/source/forward_error_correction_internal.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_header_extension_map.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_header_extensions.cc',
      'libwebrtc/rtc_base/time_utils.cc',
      'libwebrtc/rtc_base/string_utils.cc',
      'libwebrtc/rtc_base/string_encode.cc',
      'libwebrtc/rtc_base/strings/string_builder.cc',
      'libwebrtc/rtc_base/platform_thread_types.cc',
      'libwebrtc/rtc_base/strings/audio_format_to_string.cc',
      'libwebrtc/api/task_queue/task_queue_base.cc',
      'libwebrtc/modules/rtp_rtcp/source/fec_private_tables_random.cc',
      'libwebrtc/modules/rtp_rtcp/source/fec_private_tables_bursty.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_dependency_descriptor_extension.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.cc',
      'libwebrtc/modules/rtp_rtcp/source/rtp_packet_received.cc',
      'libwebrtc/common_audio/signal_processing/spl_sqrt.cc',
      'libwebrtc/common_audio/signal_processing/randomization_functions.cc',
      'libwebrtc/common_audio/signal_processing/vector_scaling_operations.cc',
      'libwebrtc/common_audio/signal_processing/filter_ar.cc',
      'libwebrtc/common_audio/signal_processing/energy.cc',
      'libwebrtc/common_audio/signal_processing/division_operations.cc',
      'libwebrtc/common_audio/signal_processing/get_hanning_window.cc',
      'libwebrtc/common_audio/signal_processing/ilbc_specific_functions.cc',
      'libwebrtc/common_audio/signal_processing/auto_correlation.cc',
      'libwebrtc/common_audio/signal_processing/levinson_durbin.cc',
      'libwebrtc/common_audio/signal_processing/copy_set_operations.cc',
      'libwebrtc/common_audio/signal_processing/get_scaling_square.cc',
      'libwebrtc/common_audio/signal_processing/min_max_operations.cc',
      'libwebrtc/common_audio/signal_processing/spl_init.cc',
      'libwebrtc/common_audio/signal_processing/cross_correlation.cc',
      'libwebrtc/common_audio/signal_processing/downsample_fast.cc',
      # C++ include files.
      'libwebrtc/system_wrappers/source/field_trial.h',
      'libwebrtc/rtc_base/rate_statistics.h',
      'libwebrtc/rtc_base/experiments/field_trial_parser.h',
      'libwebrtc/rtc_base/experiments/field_trial_units.h',
      'libwebrtc/rtc_base/experiments/alr_experiment.h',
      'libwebrtc/rtc_base/experiments/rate_control_settings.h',
      'libwebrtc/rtc_base/network/sent_packet.h',
      'libwebrtc/rtc_base/units/unit_base.h',
      'libwebrtc/rtc_base/constructor_magic.h',
      'libwebrtc/rtc_base/numerics/safe_minmax.h',
      'libwebrtc/rtc_base/numerics/safe_conversions.h',
      'libwebrtc/rtc_base/numerics/safe_conversions_impl.h',
      'libwebrtc/rtc_base/numerics/percentile_filter.h',
      'libwebrtc/rtc_base/numerics/safe_compare.h',
      'libwebrtc/rtc_base/system/unused.h',
      'libwebrtc/rtc_base/type_traits.h',
      'libwebrtc/call/rtp_transport_controller_send.h',
      'libwebrtc/call/rtp_transport_controller_send_interface.h',
      'libwebrtc/api/transport/webrtc_key_value_config.h',
      'libwebrtc/api/transport/network_types.h',
      'libwebrtc/api/transport/bitrate_settings.h',
      'libwebrtc/api/transport/network_control.h',
      'libwebrtc/api/transport/field_trial_based_config.h',
      'libwebrtc/api/transport/goog_cc_factory.h',
      'libwebrtc/api/bitrate_constraints.h',
      'libwebrtc/api/units/frequency.h',
      'libwebrtc/api/units/data_size.h',
      'libwebrtc/api/units/time_delta.h',
      'libwebrtc/api/units/data_rate.h',
      'libwebrtc/api/units/timestamp.h',
      'libwebrtc/api/network_state_predictor.h',
      'libwebrtc/modules/include/module_common_types_public.h',
      'libwebrtc/modules/pacing/interval_budget.h',
      'libwebrtc/modules/pacing/paced_sender.h',
      'libwebrtc/modules/pacing/packet_router.h',
      'libwebrtc/modules/pacing/bitrate_prober.h',
      'libwebrtc/modules/remote_bitrate_estimator/inter_arrival.h',
      'libwebrtc/modules/remote_bitrate_estimator/overuse_detector.h',
      'libwebrtc/modules/remote_bitrate_estimator/overuse_estimator.h',
      'libwebrtc/modules/remote_bitrate_estimator/bwe_defines.h',
      'libwebrtc/modules/remote_bitrate_estimator/aimd_rate_control.h',
      'libwebrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_abs_send_time.h',
      'libwebrtc/modules/remote_bitrate_estimator/include/remote_bitrate_estimator.h',
      'libwebrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_packet/transport_feedback.h',
      'libwebrtc/modules/bitrate_controller/loss_based_bandwidth_estimation.h',
      'libwebrtc/modules/bitrate_controller/send_side_bandwidth_estimation.h',
      'libwebrtc/modules/congestion_controller/goog_cc/bitrate_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/link_capacity_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/median_slope_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/probe_controller.h',
      'libwebrtc/modules/congestion_controller/goog_cc/trendline_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/goog_cc_network_control.h',
      'libwebrtc/modules/congestion_controller/goog_cc/delay_increase_detector_interface.h',
      'libwebrtc/modules/congestion_controller/goog_cc/acknowledged_bitrate_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/congestion_window_pushback_controller.h',
      'libwebrtc/modules/congestion_controller/goog_cc/delay_based_bwe.h',
      'libwebrtc/modules/congestion_controller/goog_cc/probe_bitrate_estimator.h',
      'libwebrtc/modules/congestion_controller/goog_cc/alr_detector.h',
      'libwebrtc/modules/congestion_controller/rtp/send_time_history.h',
      'libwebrtc/modules/congestion_controller/rtp/transport_feedback_adapter.h',
      'libwebrtc/modules/congestion_controller/rtp/control_handler.h',
      'libwebrtc/api/video/color_space.cc',
      'libwebrtc/mp_helpers.h',
      # C++ include files fec.
      'libwebrtc/modules/rtp_rtcp/source/byte_io.h',
      'libwebrtc/modules/rtp_rtcp/source/flexfec_header_reader_writer.h',
      'libwebrtc/modules/rtp_rtcp/source/ulpfec_header_reader_writer.h',
      'libwebrtc/modules/rtp_rtcp/source/forward_error_correction.h',
      'libwebrtc/modules/rtp_rtcp/source/forward_error_correction_internal.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_packet_received.h',
      'libwebrtc/modules/rtp_rtcp/source/ulpfec_receiver_impl.h',
      'libwebrtc/modules/rtp_rtcp/include/flexfec_receiver.h',
      'libwebrtc/modules/rtp_rtcp/include/rtp_header_extension_map.h',
      'libwebrtc/modules/rtp_rtcp/include/ulpfec_receiver.h',
      'libwebrtc/modules/audio_coding/neteq/packet.h',
      'libwebrtc/modules/audio_coding/neteq/red_payload_splitter.h',
      'libwebrtc/modules/audio_coding/neteq/decoder_database.h',
      'libwebrtc/modules/audio_coding/codecs/cng/webrtc_cng.h',
      'libwebrtc/api/rtp_headers.h',
      'libwebrtc/api/array_view.h',
      'libwebrtc/api/media_types.h',
      'libwebrtc/api/audio_options.h',
      'libwebrtc/system_wrappers/include/clock.h',
      'libwebrtc/api/media_stream_interface.h',
      'libwebrtc/api/ref_counted_base.h',
      'libwebrtc/api/rtp_packet_info.h',
      'libwebrtc/api/rtp_packet_infos.h',
      'libwebrtc/api/rtp_parameters.h',
      'libwebrtc/api/scoped_refptr.h',
      'libwebrtc/api/video/hdr_metadata.h',
      'libwebrtc/api/video/video_content_type.h',
      'libwebrtc/api/audio_codecs/audio_format.h',
      'libwebrtc/api/audio_codecs/audio_decoder_factory.h',
      'libwebrtc/api/audio_codecs/audio_decoder.h',
      'libwebrtc/api/audio_codecs/audio_codec_pair_id.h',
      'libwebrtc/api/neteq/tick_timer.h',
      'libwebrtc/rtc_base/atomic_ops.h',
      'libwebrtc/rtc_base/buffer.h',
      'libwebrtc/rtc_base/checks.h',
      'libwebrtc/rtc_base/copy_on_write_buffer.h',
      'libwebrtc/rtc_base/critical_section.h',
      'libwebrtc/rtc_base/ref_counter.h',
      'libwebrtc/rtc_base/zero_memory.h',
      'libwebrtc/rtc_base/synchronization/sequence_checker.h',
      'libwebrtc/rtc_base/synchronization/rw_lock_wrapper.h',
      'libwebrtc/rtc_base/synchronization/rw_lock_posix.h',
      'libwebrtc/rtc_base/strings/audio_format_to_string.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_packet.h',
      'libwebrtc/modules/rtp_rtcp/source/forward_error_correction_internal.h',
      'libwebrtc/modules/rtp_rtcp/include/rtp_header_extension_map.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_header_extensions.h',
      'libwebrtc/rtc_base/time_utils.h',
      'libwebrtc/rtc_base/string_utils.h',
      'libwebrtc/rtc_base/string_encode.h',
      'libwebrtc/rtc_base/strings/string_builder.h',
      'libwebrtc/rtc_base/platform_thread_types.h',
      'libwebrtc/api/task_queue/task_queue_base.h',
      'libwebrtc/api/video/color_space.h',
      'libwebrtc/modules/rtp_rtcp/source/fec_private_tables_random.h',
      'libwebrtc/modules/rtp_rtcp/source/fec_private_tables_bursty.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_dependency_descriptor_extension.h',
      'libwebrtc/modules/rtp_rtcp/source/rtp_generic_frame_descriptor_extension.h',
      'libwebrtc/common_types.h',
      'libwebrtc/common_audio/signal_processing/include/signal_processing_library.h'
    ],
    'include_dirs':
    [
      'libwebrtc',
      '../../include',
      '../json/single_include/nlohmann'
    ],
    'conditions':
    [
      # Endianness.
      [ 'node_byteorder == "big"', {
          # Define Big Endian.
          'defines': [ 'MS_BIG_ENDIAN' ]
        }, {
          # Define Little Endian.
          'defines': [ 'MS_LITTLE_ENDIAN' ]
      }],

      # Platform-specifics.

      [ 'OS != "win"', {
        "sources": [
          "libwebrtc/rtc_base/system_wrappers/rw_lock_win.cc"
        ],
        'cflags': [ '-std=c++11' ]
      }],

      [ 'OS == "mac"', {
        'xcode_settings':
        {
          'OTHER_CPLUSPLUSFLAGS' : [ '-std=c++11' ]
        }
      }]
    ]
  },
  'targets':
  [
    {
      'target_name': 'libwebrtc',
      'type': 'static_library'
    }
  ]
}

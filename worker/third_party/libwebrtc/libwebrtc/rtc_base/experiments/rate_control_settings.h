/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_EXPERIMENTS_RATE_CONTROL_SETTINGS_H_
#define RTC_BASE_EXPERIMENTS_RATE_CONTROL_SETTINGS_H_

#include "api/transport/webrtc_key_value_config.h"
#include "rtc_base/experiments/field_trial_parser.h"
// #include "rtc_base/experiments/field_trial_units.h"
#include "api/video_codecs/video_codec.h"

#include <absl/types/optional.h>

namespace webrtc {

struct VideoRateControlConfig {
  static constexpr char kKey[] = "WebRTC-VideoRateControl";
  absl::optional<double> pacing_factor;
  bool alr_probing = false;
  absl::optional<int> vp8_qp_max;
  absl::optional<int> vp8_min_pixels;
  bool trust_vp8 = false;
  bool trust_vp9 = false;
  double video_hysteresis = 1.0;
  // Default to 35% hysteresis for simulcast screenshare.
  double screenshare_hysteresis = 1.35;
  bool probe_max_allocation = true;
  bool bitrate_adjuster = false;
  bool adjuster_use_headroom = false;
  bool vp8_s0_boost = true;
  bool vp8_dynamic_rate = false;
  bool vp9_dynamic_rate = false;
};

class RateControlSettings final {
 public:
  ~RateControlSettings();
  RateControlSettings(RateControlSettings&&);

  static RateControlSettings ParseFromFieldTrials();
  static RateControlSettings ParseFromKeyValueConfig(
      const WebRtcKeyValueConfig* const key_value_config);

  // When CongestionWindowPushback is enabled, the pacer is oblivious to
  // the congestion window. The relation between outstanding data and
  // the congestion window affects encoder allocations directly.
  bool UseCongestionWindow() const;
  int64_t GetCongestionWindowAdditionalTimeMs() const;
  bool UseCongestionWindowPushback() const;
  uint32_t CongestionWindowMinPushbackTargetBitrateBps() const;

  absl::optional<double> GetPacingFactor() const;
  bool UseAlrProbing() const;

  bool TriggerProbeOnMaxAllocatedBitrateChange() const;
  bool UseEncoderBitrateAdjuster() const;
  bool BitrateAdjusterCanUseNetworkHeadroom() const;

  double GetSimulcastHysteresisFactor(VideoCodecMode mode) const;

 private:
  explicit RateControlSettings(
      const WebRtcKeyValueConfig* const key_value_config);

  double GetSimulcastScreenshareHysteresisFactor() const;

  FieldTrialOptional<int> congestion_window_;
  FieldTrialOptional<int> congestion_window_pushback_;
  FieldTrialOptional<double> pacing_factor_;
  FieldTrialParameter<bool> alr_probing_;
  FieldTrialParameter<bool> probe_max_allocation_;
  FieldTrialParameter<bool> bitrate_adjuster_;
  FieldTrialParameter<bool> adjuster_use_headroom_;

  VideoRateControlConfig video_config_;
};

}  // namespace webrtc

#endif  // RTC_BASE_EXPERIMENTS_RATE_CONTROL_SETTINGS_H_

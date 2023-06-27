/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_
#define MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_

#include <stddef.h>
#include <stdint.h>

#include <deque>
#include <utility>

#include "api/network_state_predictor.h"
#include "api/transport/webrtc_key_value_config.h"
#include "modules/congestion_controller/goog_cc/delay_increase_detector_interface.h"
#include "modules/remote_bitrate_estimator/include/bwe_defines.h"
#include "rtc_base/constructor_magic.h"

namespace webrtc {

class TrendlineEstimator : public DelayIncreaseDetectorInterface {
 public:
  TrendlineEstimator(const WebRtcKeyValueConfig* key_value_config,
                     NetworkStatePredictor* network_state_predictor);
  // |window_size| is the number of points required to compute a trend line.
  // |smoothing_coef| controls how much we smooth out the delay before fitting
  // the trend line. |threshold_gain| is used to scale the trendline slope for
  // comparison to the old threshold. Once the old estimator has been removed
  // (or the thresholds been merged into the estimators), we can just set the
  // threshold instead of setting a gain.|network_state_predictor| is used to
  // bettter predict network state.
  TrendlineEstimator(size_t window_size, double smoothing_coef,
                     double threshold_gain,
                     NetworkStatePredictor* network_state_predictor);

  ~TrendlineEstimator() override;

  // Update the estimator with a new sample. The deltas should represent deltas
  // between timestamp groups as defined by the InterArrival class.
  void Update(double recv_delta_ms, double send_delta_ms, int64_t send_time_ms,
              int64_t arrival_time_ms, bool calculated_deltas) override;

  BandwidthUsage State() const override;

  void ChangeWindowSize(size_t size) {
    if (size > 150) {
      window_size_ = 150;
    } else if (size < 40) {
      window_size_ = 40;
    } else {
      window_size_ = size;
    }
  }

  void ChangeDynamicMinThreshold(double threshold) {
    if (threshold > 90.f) {
      dynamic_min_threshold_ = 90.f;
    } else if (threshold < 6.f) {
      dynamic_min_threshold_ = 6.f;
    } else {
      dynamic_min_threshold_ = threshold;
    }
  }

  std::vector<double> GetPrevTrend() {
    auto temp = this->trends_;
    this->trends_.clear();
    return temp;
  }

 protected:
  // Used in unit tests.
  double modified_trend() const { return prev_trend_ * threshold_gain_; }

 private:
  friend class GoogCcStatePrinter;

  void Detect(double trend, double ts_delta, int64_t now_ms);

  void UpdateThreshold(double modified_offset, int64_t now_ms);

  // Parameters.
  size_t window_size_;
  double dynamic_min_threshold_;
  const double smoothing_coef_;
  const double threshold_gain_;
  // Used by the existing threshold.
  int num_of_deltas_;
  // Keep the arrival times small by using the change from the first packet.
  int64_t first_arrival_time_ms_;
  // Exponential backoff filtering.
  double accumulated_delay_;
  double smoothed_delay_;
  // Linear least squares regression.
  std::deque<std::pair<double, double>> delay_hist_;

  const double k_up_;
  const double k_down_;
  double overusing_time_threshold_;
  double threshold_;
  double prev_modified_trend_;
  int64_t last_update_ms_;
  double prev_trend_;
  std::vector<double> trends_;
  double time_over_using_;
  int overuse_counter_;
  BandwidthUsage hypothesis_;
  BandwidthUsage hypothesis_predicted_;
  NetworkStatePredictor* network_state_predictor_;

  RTC_DISALLOW_COPY_AND_ASSIGN(TrendlineEstimator);
};
}  // namespace webrtc

#endif  // MODULES_CONGESTION_CONTROLLER_GOOG_CC_TRENDLINE_ESTIMATOR_H_

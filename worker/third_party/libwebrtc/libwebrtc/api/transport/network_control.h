/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_TRANSPORT_NETWORK_CONTROL_H_
#define API_TRANSPORT_NETWORK_CONTROL_H_

#include <stdint.h>

#include <memory>

#include "api/transport/network_types.h"
#include "api/transport/webrtc_key_value_config.h"

namespace webrtc {
// TODO(srte): Remove this forward declaration when this is in api.
class RtcEventLog;

class TargetTransferRateObserver {
 public:
  virtual ~TargetTransferRateObserver() = default;
  // Called to indicate target transfer rate as well as giving information about
  // the current estimate of network parameters.
  virtual void OnTargetTransferRate(TargetTransferRate) = 0;
  // Called to provide updates to the expected target rate in case it changes
  // before the first call to OnTargetTransferRate.
  virtual void OnStartRateUpdate(DataRate) {}
};

// Configuration sent to factory create function. The parameters here are
// optional to use for a network controller implementation.
struct NetworkControllerConfig {
  // The initial constraints to start with, these can be changed at any later
  // time by calls to OnTargetRateConstraints. Note that the starting rate
  // has to be set initially to provide a starting state for the network
  // controller, even though the field is marked as optional.
  TargetRateConstraints constraints;
  // Initial stream specific configuration, these are changed at any later time
  // by calls to OnStreamsConfig.
  StreamsConfig stream_based_config;

  // Optional override of configuration of WebRTC internals. Using nullptr here
  // indicates that the field trial API will be used.
  const WebRtcKeyValueConfig* key_value_config = nullptr;
  // Optional override of event log.
  RtcEventLog* event_log = nullptr;
};

// NetworkControllerInterface is implemented by network controllers. A network
// controller is a class that uses information about network state and traffic
// to estimate network parameters such as round trip time and bandwidth. Network
// controllers does not guarantee thread safety, the interface must be used in a
// non-concurrent fashion.
class NetworkControllerInterface {
 public:
  virtual ~NetworkControllerInterface() = default;

  // Called when network availabilty changes.
  virtual NetworkControlUpdate OnNetworkAvailability(NetworkAvailability) = 0;
  // Called when the receiving or sending endpoint changes address.
  virtual NetworkControlUpdate OnNetworkRouteChange(NetworkRouteChange) = 0;
  // Called periodically with a periodicy as specified by
  // NetworkControllerFactoryInterface::GetProcessInterval.
  virtual NetworkControlUpdate OnProcessInterval(ProcessInterval) = 0;
  // Called when remotely calculated bitrate is received.
  virtual NetworkControlUpdate OnRemoteBitrateReport(RemoteBitrateReport) = 0;
  // Called round trip time has been calculated by protocol specific mechanisms.
  virtual NetworkControlUpdate OnRoundTripTimeUpdate(RoundTripTimeUpdate) = 0;
  // Called when a packet is sent on the network.
  virtual NetworkControlUpdate OnSentPacket(SentPacket) = 0;
  // Called when the stream specific configuration has been updated.
  virtual NetworkControlUpdate OnStreamsConfig(StreamsConfig) = 0;
  // Called when target transfer rate constraints has been changed.
  virtual NetworkControlUpdate OnTargetRateConstraints(
      TargetRateConstraints) = 0;
  // Called when a protocol specific calculation of packet loss has been made.
  virtual NetworkControlUpdate OnTransportLossReport(TransportLossReport) = 0;
  // Called with per packet feedback regarding receive time.
  virtual NetworkControlUpdate OnTransportPacketsFeedback(
      TransportPacketsFeedback) = 0;
  // Called with network state estimate updates.
  virtual NetworkControlUpdate OnNetworkStateEstimate(NetworkStateEstimate) = 0;

  virtual void SetSendSideBandwidthMinBitrate(int minBitrate) {}

  virtual void ChangeWindowSize(size_t size) {}

  virtual void ChangeDynamicMinThreshold(double threshold) {}

  virtual std::vector<double> GetPrevTrend() {
    std::vector<double> result;
    return result;
  }

  virtual void SetNoBitrateIncreaseInAlr(bool flag) {}
};

// NetworkControllerFactoryInterface is an interface for creating a network
// controller.
class NetworkControllerFactoryInterface {
 public:
  virtual ~NetworkControllerFactoryInterface() = default;

  // Used to create a new network controller, requires an observer to be
  // provided to handle callbacks.
  virtual std::unique_ptr<NetworkControllerInterface> Create(
      NetworkControllerConfig config, bifrost::UvLoop* loop) = 0;
  // Returns the interval by which the network controller expects
  // OnProcessInterval calls.
  virtual TimeDelta GetProcessInterval() const = 0;
};

// Under development, subject to change without notice.
class NetworkStateEstimator {
 public:
  // Gets the current best estimate according to the estimator.
  virtual absl::optional<NetworkStateEstimate> GetCurrentEstimate() = 0;
  // Called with per packet feedback regarding receive time.
  virtual void OnTransportPacketsFeedback(const TransportPacketsFeedback&) = 0;
  // Called when the receiving or sending endpoint changes address.
  virtual void OnRouteChange(const NetworkRouteChange&) = 0;
  virtual ~NetworkStateEstimator() = default;
};
class NetworkStateEstimatorFactory {
 public:
  virtual std::unique_ptr<NetworkStateEstimator> Create(
      const WebRtcKeyValueConfig* key_value_config) = 0;
  virtual ~NetworkStateEstimatorFactory() = default;
};
}  // namespace webrtc

#endif  // API_TRANSPORT_NETWORK_CONTROL_H_

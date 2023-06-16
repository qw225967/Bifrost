#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV_LEVEL 3

#include "tcc_client.hpp"

#include <api/transport/network_types.h>  // webrtc::TargetRateConstraints

#include <limits>

#include "uv_loop.h"

namespace bifrost {
/* Static. */
static constexpr uint32_t MinBitrate{30000u};
static constexpr float MaxBitrateMarginFactor{0.1f};
static constexpr float MaxBitrateIncrementFactor{1.35f};
static constexpr float MaxPaddingBitrateFactor{0.85f};
static constexpr uint64_t AvailableBitrateEventInterval{1000u};  // In ms.
static constexpr size_t PacketLossHistogramLength{24};

/* Instance methods. */
TransportCongestionControlClient::TransportCongestionControlClient(
    TransportCongestionControlClient::Listener* listener,
    uint32_t initialAvailableBitrate, uint32_t maxOutgoingBitrate,
    UvLoop* uv_loop)
    : listener(listener),
      initialAvailableBitrate(
          std::max<uint32_t>(initialAvailableBitrate, MinBitrate)),
      maxOutgoingBitrate(maxOutgoingBitrate),
      uv_loop_(uv_loop) {
  webrtc::GoogCcFactoryConfig config;

  // Provide RTCP feedback as well as Receiver Reports.
  config.feedback_only = false;

  this->controllerFactory =
      new webrtc::GoogCcNetworkControllerFactory(std::move(config));
}

TransportCongestionControlClient::~TransportCongestionControlClient() {
  delete this->controllerFactory;
  this->controllerFactory = nullptr;

  DestroyController();
}

void TransportCongestionControlClient::InitializeController() {
  webrtc::BitrateConstraints bitrateConfig;
  bitrateConfig.start_bitrate_bps =
      static_cast<int>(this->initialAvailableBitrate);

  this->rtpTransportControllerSend = new webrtc::RtpTransportControllerSend(
      this, nullptr, this->controllerFactory, bitrateConfig);

  this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

  // This makes sure that periodic probing is used when the application is send
  // less bitrate than needed to measure the bandwidth estimation.  (f.e. when
  // videos are muted or using screensharing with still images)
  this->rtpTransportControllerSend->EnablePeriodicAlrProbing(true);

  this->processTimer = new UvTimer(this, uv_loop_->get_loop().get());

  this->processTimer->Start(std::min(
      // Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field
      // trial.
      this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
      // Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
      this->controllerFactory->GetProcessInterval().ms()));

  this->rtpTransportControllerSend->OnNetworkAvailability(true);
}

void TransportCongestionControlClient::DestroyController() {
  delete this->rtpTransportControllerSend;
  this->rtpTransportControllerSend = nullptr;

  delete this->processTimer;
  this->processTimer = nullptr;

  this->rtpTransportControllerSend->OnNetworkAvailability(false);
}

void TransportCongestionControlClient::InsertPacket(
    webrtc::RtpPacketSendInfo& packetInfo) {
  if (this->rtpTransportControllerSend == nullptr) {
    return;
  }

  this->rtpTransportControllerSend->packet_sender()->InsertPacket(
      packetInfo.length);
  this->rtpTransportControllerSend->OnAddPacket(packetInfo);
}

webrtc::PacedPacketInfo TransportCongestionControlClient::GetPacingInfo() {
  if (this->rtpTransportControllerSend == nullptr) {
    return {};
  }

  return this->rtpTransportControllerSend->packet_sender()->GetPacingInfo();
}

void TransportCongestionControlClient::PacketSent(
    webrtc::RtpPacketSendInfo& packetInfo, int64_t nowMs) {
  if (this->rtpTransportControllerSend == nullptr) {
    return;
  }

  // Notify the transport feedback adapter about the sent packet.
  rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, nowMs);
  this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);
}

void TransportCongestionControlClient::ReceiveEstimatedBitrate(
    uint32_t bitrate) {
  if (this->rtpTransportControllerSend == nullptr) {
    return;
  }

  this->rtpTransportControllerSend->OnReceivedEstimatedBitrate(bitrate);
}

void TransportCongestionControlClient::ReceiveRtcpTransportFeedback(
    const RTC::RTCP::FeedbackRtpTransportPacket* feedback) {
  // Update packet loss history.
  size_t expected_packets = feedback->GetPacketStatusCount();
  size_t lost_packets = 0;
  for (const auto& result : feedback->GetPacketResults()) {
    if (!result.received) lost_packets += 1;
  }
  this->UpdatePacketLoss(static_cast<double>(lost_packets) / expected_packets);

  if (this->rtpTransportControllerSend == nullptr) {
    return;
  }

  this->rtpTransportControllerSend->OnTransportFeedback(*feedback);
}

void TransportCongestionControlClient::UpdatePacketLoss(double packetLoss) {
  // Add the score into the histogram.
  if (this->packetLossHistory.size() == PacketLossHistogramLength)
    this->packetLossHistory.pop_front();

  this->packetLossHistory.push_back(packetLoss);

  /*
   * Scoring mechanism is a weighted average.
   *
   * The more recent the score is, the more weight it has.
   * The oldest score has a weight of 1 and subsequent scores weight is
   * increased by one sequentially.
   *
   * Ie:
   * - scores: [1,2,3,4]
   * - this->scores = ((1) + (2+2) + (3+3+3) + (4+4+4+4)) / 10 = 2.8 => 3
   */

  size_t weight{0};
  size_t samples{0};
  double totalPacketLoss{0};

  for (auto packetLossEntry : this->packetLossHistory) {
    weight++;
    samples += weight;
    totalPacketLoss += weight * packetLossEntry;
  }

  // clang-tidy "thinks" that this can lead to division by zero but we are
  // smarter.
  // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
  this->packetLoss = totalPacketLoss / samples;
}

void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent() {
  this->lastAvailableBitrateEventAtMs = this->uv_loop_->get_time_ms();
}

void TransportCongestionControlClient::MayEmitAvailableBitrateEvent(
    uint32_t previousAvailableBitrate) {
  uint64_t nowMs = this->uv_loop_->get_time_ms_int64();
  bool notify{false};

  // Ignore if first event.
  // NOTE: Otherwise it will make the Transport crash since this event also
  // happens during the constructor of this class.
  if (this->lastAvailableBitrateEventAtMs == 0u) {
    this->lastAvailableBitrateEventAtMs = nowMs;

    return;
  }

  // Emit if this is the first valid event.
  if (!this->availableBitrateEventCalled) {
    this->availableBitrateEventCalled = true;

    notify = true;
  }
  // Emit event if AvailableBitrateEventInterval elapsed.
  else if (nowMs - this->lastAvailableBitrateEventAtMs >=
           AvailableBitrateEventInterval) {
    notify = true;
  }
  // Also emit the event fast if we detect a high BWE value decrease.
  else if (this->bitrates.availableBitrate < previousAvailableBitrate * 0.75) {
    std::cout << "[tcc client] high BWE value decrease detected, notifying the "
                 "listener [now:"
              << this->bitrates.availableBitrate << ", before:%"
              << previousAvailableBitrate << "]" << std::endl;

    notify = true;
  }
  // Also emit the event fast if we detect a high BWE value increase.
  else if (this->bitrates.availableBitrate > previousAvailableBitrate * 1.50) {
    std::cout << "[tcc client] high BWE value increase detected, notifying the "
                 "listener [now:"
              << this->bitrates.availableBitrate << ", before:%"
              << previousAvailableBitrate << "]" << std::endl;

    notify = true;
  }

  if (notify) {
    std::cout
        << "[tcc client] notifying the listener with new available bitrate:"
        << this->bitrates.availableBitrate << std::endl;

    this->lastAvailableBitrateEventAtMs = nowMs;

    this->listener->OnTransportCongestionControlClientBitrates(this,
                                                               this->bitrates);
  }
}

void TransportCongestionControlClient::OnTargetTransferRate(
    webrtc::TargetTransferRate targetTransferRate) {
  // NOTE: The same value as 'this->initialAvailableBitrate' is received
  // periodically regardless of the real available bitrate. Skip such value
  // except for the first time this event is called.
  // clang-format off
		if (
			this->availableBitrateEventCalled &&
			targetTransferRate.target_rate.bps() == this->initialAvailableBitrate
		)
  // clang-format on
  {
    return;
  }

  auto previousAvailableBitrate = this->bitrates.availableBitrate;

  // Update availableBitrate.
  // NOTE: Just in case.
  if (targetTransferRate.target_rate.bps() >
      std::numeric_limits<uint32_t>::max())
    this->bitrates.availableBitrate = std::numeric_limits<uint32_t>::max();
  else
    this->bitrates.availableBitrate =
        static_cast<uint32_t>(targetTransferRate.target_rate.bps());

  std::cout << "[tcc client] new available bitrate:"
            << this->bitrates.availableBitrate << std::endl;

  MayEmitAvailableBitrateEvent(previousAvailableBitrate);
}

// Called from PacedSender in order to send probation packets.
void TransportCongestionControlClient::SendPacket(
    RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo) {
  // Send the packet.
  this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet,
                                                                  pacingInfo);
}

void TransportCongestionControlClient::OnTimer(UvTimer* timer) {
  if (timer == this->processTimer) {
    // Time to call RtpTransportControllerSend::Process().
    this->rtpTransportControllerSend->Process();

    // Time to call PacedSender::Process().
    this->rtpTransportControllerSend->packet_sender()->Process();

    /* clang-format off */
			this->processTimer->Start(std::min<uint64_t>(
				// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
				this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
				// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
				this->controllerFactory->GetProcessInterval().ms()
			));
    /* clang-format on */

    MayEmitAvailableBitrateEvent(this->bitrates.availableBitrate);
  }
}
}  // namespace bifrost
